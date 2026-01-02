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

namespace jmg::uring
{

Event::Event(io_uring* ring, io_uring_cqe* cqe) : ring_(ring), cqe_(cqe) {
  if (cqe) {
    JMG_ENFORCE_USING(
      std::logic_error, pred(ring),
      "received a non-null CQE pointer with a null ring pointer");
  }
}

Event::~Event() {
  if (cqe_ && !is_cleanup_canceled_) { io_uring_cqe_seen(ring_, cqe_); }
}

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

Uring::~Uring() { io_uring_queue_exit(&ring_); }

Event Uring::awaitEvent(optional<Duration> timeout) {
  unique_ptr<UringDuration> wait_timeout;
  string is_timeout_str;
  io_uring_cqe* cqe = nullptr;
  if (timeout) {
    UringDuration duration = from(*timeout);
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
  clear_notifier.cancel();
}

void Uring::submitTimeoutReq(UserData data,
                             Duration timeout,
                             DelaySubmission isDelayed) {
  auto& sqe = getNextSqe();
  io_uring_sqe_set_data64(&sqe, unwrap(data));
  UringDuration timeout_duration = from(timeout);
  io_uring_prep_timeout(&sqe, &timeout_duration, 0 /* count */,
                        IORING_TIMEOUT_ETIME_SUCCESS);
  if (!unwrap(isDelayed)) { submitReq("timeout"sv); }
}

void Uring::submitWriteReq(const int fd,
                           const IoVecView io_vec,
                           DelaySubmission is_delayed,
                           optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  // NOTE: offset is always 0 since io_vec is a std::span and can be used to
  // generate an offset into a larger collection of iovec structures if needed
  io_uring_prep_writev(&sqe, fd, io_vec.data(), io_vec.size(), 0 /* offset */);
  if (user_data) {
    io_uring_sqe_set_data64(&sqe, static_cast<__u64>(unsafe(*user_data)));
  }
  if (!unwrap(is_delayed)) { submitReq("write"sv); }
}

void Uring::submitReadReq(const int fd,
                          const IoVecView io_vec,
                          DelaySubmission is_delayed,
                          optional<UserData> user_data) {
  auto& sqe = getNextSqe();
  // NOTE: offset is always 0 since io_vec is a std::span and can be used to
  // generate an offset into a larger collection of iovec structures if needed
  io_uring_prep_readv(&sqe, fd, io_vec.data(), io_vec.size(), 0 /* offset */);
  if (user_data) {
    io_uring_sqe_set_data64(&sqe, static_cast<__u64>(unsafe(*user_data)));
  }
  if (!unwrap(is_delayed)) { submitReq("read"sv); }
}

io_uring_sqe& Uring::getNextSqe() {
  auto* sqe = io_uring_get_sqe(&ring_);
  // TODO(bd) allow SQEs to be stored on a pending queue to be submitted when
  // the next ring slot becomes available
  JMG_ENFORCE(pred(sqe), "no submit queue entries currently available");
  return *sqe;
}

} // namespace jmg::uring
