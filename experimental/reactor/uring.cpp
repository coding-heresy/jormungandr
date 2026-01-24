/** -*- mode: c++ -*-
 *
 * Copyright (C) 2025 Brian Davis
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Brian Davis <brian8702@sbcglobal.net>
 *
 */

#include <memory>
#include <optional>

#include "jmg/conversion.h"
#include "jmg/preprocessor.h"
#include "uring.h"

using namespace std;
using namespace std::string_view_literals;

#define JMG_URING_STORE_USER_DATA(sq, ud) \
  io_uring_sqe_set_data64(&(sq), static_cast<__u64>(unsafe(ud)))

namespace jmg::uring
{

Event::Event(io_uring* ring, io_uring_cqe* cqe) : ring_(ring), cqe_(cqe) {
  if (cqe) {
    JMG_ENFORCE_USING(
      std::logic_error, pred(ring),
      "received a non-null CQE pointer with a null ring pointer");
  }
}

Event::Event(Event&& src) {
  this->ring_ = src.ring_;
  this->cqe_ = src.cqe_;
  src.ring_ = nullptr;
  src.cqe_ = nullptr;
}

Event& Event::operator=(Event&& src) {
  if (this == &src) { return *this; }
  this->ring_ = src.ring_;
  this->cqe_ = src.cqe_;
  src.ring_ = nullptr;
  src.cqe_ = nullptr;
  return *this;
}

Event::~Event() {
  if (ring_ && cqe_) { io_uring_cqe_seen(ring_, cqe_); }
}

Uring::~Uring() { io_uring_queue_exit(&ring_); }

Uring::Uring(const UringSz sz) {
  memset(&ring_, 0, sizeof(ring_));
  io_uring_params params = {};

  // TODO(bd) are these flags correct?

  // only the reactor main thread should access
  params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_COOP_TASKRUN
                 | IORING_SETUP_DEFER_TASKRUN;
  JMG_SYSTEM(io_uring_queue_init_params(unsafe(sz), &ring_, &params),
             "unable to initialize io_uring");
  // save the channel file descriptor so other threads with access to separate
  // urings can send in messages
  channel_ = FileDescriptor(ring_.ring_fd);
}

bool Uring::hasEvent() {
  io_uring_cqe* cqe = nullptr;
  const auto rc = io_uring_peek_cqe(&ring_, &cqe);
  if (!rc) { return true; }
  if (-EAGAIN == rc) { return false; }
  JMG_SYSTEM_ERRNO_RETURN(rc, "unable to poll for completion queue event");
  // TODO(bd) should be unreachable?
  return false;
}

Event Uring::awaitEvent(optional<Duration> timeout) {
  unique_ptr<UringTimeSpec> wait_timeout;
  string is_timeout_str;
  io_uring_cqe* cqe = nullptr;

  // clean up old debug messages when they are no longer needed
  using Cleaner = function<void()>;
  auto debug_msgs_maintenance = [&]() -> optional<Cleanup<Cleaner>> {
    if (debug_msgs_.empty()) { return nullopt; }
    auto maybe_clean = [&] {
      if (!io_uring_sq_ready(&ring_)) {
        // submission queue is empty, any outstanding debug messages can
        // be cleaned up
        debug_msgs_.clear();
      }
    };
    return Cleanup<Cleaner>(std::move(maybe_clean));
  }();

  if (timeout) {
    UringTimeSpec duration = from(*timeout);
    auto rc = io_uring_wait_cqe_timeout(&ring_, &cqe, &duration);
    if (-ETIME == rc) {
      // timeout is not a failure, return empty event
      return Event();
    }
    if (rc < 0) {}
    JMG_SYSTEM_ERRNO_RETURN(
      rc, "unable to wait for io_uring completion with timeout");
  }
  else {
    JMG_SYSTEM_ERRNO_RETURN(
      io_uring_wait_cqe(&ring_, &cqe),
      "unable to wait for io_uring completion with no timeout");
  }
  JMG_ENFORCE(
    pred(cqe),
    "successfully waited for uring event but no event details were returned");

  /*
    TODO(bd) decide how to handle failures of individual, long-running requests

    e.g. for multishot polling on eventfd:

    A CQE posted from a   multishot poll request will have IORING_CQE_F_MORE set
    in the CQE flags member, indicating that the application should expect more
    completions from this request. If the multishot poll request gets terminated
    or experiences an error, this flag will not be set in the CQE. If this
    happens, the application should not expect further CQEs from the original
    request and must reissue a new one if it still wishes to get notifications
    on this file descriptor.
  */

  return Event(&ring_, cqe);
}

void Uring::registerEventNotifier(EventFd notifier, DelaySubmission isDelayed) {
  JMG_ENFORCE_USING(
    logic_error, !notifier_,
    "attempted to register more than one event notifier with uring instance");
  notifier_ = notifier;
  auto clear_notifier = Cleanup([&] { notifier_ = nullopt; });
  auto sqe = io_uring_get_sqe(&ring_);
  // read readiness triggers the event
  io_uring_prep_poll_multishot(sqe, unsafe(notifier), POLL_IN);
  // use event_fd as user_data for identification
  io_uring_sqe_set_data(sqe, (void*)static_cast<intptr_t>(unsafe(notifier)));
  if (!unwrap(isDelayed)) { submitReq("event notifier registration"sv); }
  std::move(clear_notifier).cancel();
}

io_uring_sqe& Uring::getNextSqe() {
  auto* sqe = io_uring_get_sqe(&ring_);
  // TODO(bd) allow SQEs to be stored on a pending queue to be submitted when
  // the next ring slot becomes available
  JMG_ENFORCE(pred(sqe), "no submit queue entries currently available");
  return *sqe;
}

void Uring::reqFinalize(io_uring_sqe& sqe,
                        const DelaySubmission is_delayed,
                        const std::optional<UserData> user_data,
                        const std::string_view req_type) {
  if (user_data) { JMG_URING_STORE_USER_DATA(sqe, *user_data); }
  if (!unwrap(is_delayed)) { submitReq(req_type); }
}

void Uring::submitFdCloseReq(const int fd,
                             const DelaySubmission is_delayed,
                             const std::optional<UserData> user_data) {
  JMG_ENFORCE_USING(logic_error, fd > -1, "invalid file descriptor value [", fd,
                    "]");
  auto& sqe = getNextSqe();
  io_uring_prep_close(&sqe, fd);
  reqFinalize(sqe, is_delayed, user_data, "close file descriptor"sv);
}

void Uring::submitTimeoutReq(UringTimeSpec timeout_spec,
                             unsigned flags,
                             UserData user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_timeout(&sqe, &timeout_spec, 0 /* count */,
                        flags | IORING_TIMEOUT_ETIME_SUCCESS);
  JMG_URING_STORE_USER_DATA(sqe, user_data);
  submitReq("timeout"sv);
}

void Uring::submitFileOpenReq(const c_string_view file_path,
                              const FileOpenFlags flags,
                              const mode_t permissions,
                              const DelaySubmission is_delayed,
                              const std::optional<UserData> user_data) {
  JMG_ENFORCE_USING(logic_error, !file_path.empty(), "empty file path");
  auto& sqe = getNextSqe();
  io_uring_prep_openat(&sqe, AT_FDCWD, file_path.c_str(),
                       static_cast<uint16_t>(flags), permissions);
  reqFinalize(sqe, is_delayed, user_data, "open file"sv);
}

void Uring::submitNetConnectReq(const int sd,
                                const IpEndpoint& tgt_endpoint,
                                DelaySubmission is_delayed,
                                std::optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_connect(&sqe, sd, (struct sockaddr*)&(tgt_endpoint.addr()),
                        sizeof(struct sockaddr_in));
  reqFinalize(sqe, is_delayed, user_data, "connect to network endpoint"sv);
}

void Uring::submitWriteReq(const int fd,
                           const IoVecView io_vec,
                           const DelaySubmission is_delayed,
                           const optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  // NOTE: offset is always 0 since io_vec is a std::span and can be used to
  // generate an offset into a larger collection of iovec structures if needed
  io_uring_prep_writev(&sqe, fd, io_vec.data(), io_vec.size(), 0 /* offset */);
  reqFinalize(sqe, is_delayed, user_data, "write"sv);
}

void Uring::submitReadReq(const int fd,
                          const IoVecView io_vec,
                          const DelaySubmission is_delayed,
                          const optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  // NOTE: offset is always 0 since io_vec is a std::span and can be used to
  // generate an offset into a larger collection of iovec structures if needed
  io_uring_prep_readv(&sqe, fd, io_vec.data(), io_vec.size(), 0 /* offset */);
  reqFinalize(sqe, is_delayed, user_data, "read"sv);
}

void Uring::submitRecvFromReq(const int sd,
                              BufferProxy buf,
                              int flags,
                              const DelaySubmission is_delayed,
                              const optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_recv(&sqe, sd, buf.data(), buf.size(), flags);
  reqFinalize(sqe, is_delayed, user_data, "recvfrom"sv);
}

void Uring::submitSetSockOptReq(const int sd,
                                int level,
                                int opt_name,
                                const void* opt_val,
                                size_t opt_sz,
                                const DelaySubmission is_delayed,
                                const optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_cmd_sock(&sqe, SOCKET_URING_OP_SETSOCKOPT, sd, level, opt_name,
                         const_cast<void*>(opt_val), opt_sz);
  reqFinalize(sqe, is_delayed, user_data, "set socket options"sv);
}

void Uring::submitSocketBindReq(int sd,
                                const Port port,
                                DelaySubmission is_delayed,
                                std::optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(unsafe(port));
  io_uring_prep_bind(&sqe, sd, (sockaddr*)&addr, sizeof(addr));
  reqFinalize(sqe, is_delayed, user_data, "set socket options"sv);
}

void Uring::submitSocketListenReq(int sd,
                                  int backlog,
                                  DelaySubmission is_delayed,
                                  std::optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_listen(&sqe, sd, backlog);
  reqFinalize(sqe, is_delayed, user_data, "socket listen"sv);
}

#if defined(JMG_LIBURING_VERSION_SUPPORTS_GETSOCKNAME)
void Uring::submitSocketInfoReq(int sd,
                                struct sockaddr& addr,
                                socklen_t& addrSz,
                                SocketSide side,
                                DelaySubmission is_delayed,
                                std::optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  io_uring_prep_getsockname(&sqe, sd, &addr, &addrSz, unwrap(side));
  reqFinalize(sqe, is_delayed, user_data, "get socket info"sv);
}
#endif

void Uring::submitDebugLogReq(IoVecView io_vec) {
  auto& sqe = getNextSqe();
  // NOTE: offset is always 0 since io_vec is a std::span and can be used to
  // generate an offset into a larger collection of iovec structures if needed
  // TODO(bd) support writing debug output to stderr?
  io_uring_prep_writev(&sqe, unsafe(kStdoutFd), io_vec.data(), io_vec.size(),
                       0 /* offset */);
  // allow failure reports to be discarded or logged separate from
  // other completion events
  io_uring_sqe_set_data64(&sqe,
                          static_cast<__u64>(unsafe(kDetachedOperationFailure)));
  // best efforts delivery
  io_uring_sqe_set_flags(&sqe, IOSQE_CQE_SKIP_SUCCESS);
  submitReq("debug log"sv);
}

} // namespace jmg::uring
