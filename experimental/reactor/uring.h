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
#pragma once

#include <forward_list>
#include <optional>

#include <liburing.h>

#include "jmg/conversion.h"
#include "jmg/ip_endpoint.h"
#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"
#include "jmg/types.h"
#include "jmg/util.h"

// TODO(bd) io_uring_prep_getsockname is not supported until version
// 2.13 of liburing
// #define JMG_LIBURING_VERSION_SUPPORTS_GETSOCKNAME

/**
 * wrapper class for io_uring
 *
 * User Data Protocol:
 *
 * User data passed with a uring request is returned as part of the
 * completion event. This header declares a safe type UserData for
 * such data and its use is generally unrestricted except that the
 * value -1 is reserved for use in cases where the request has
 * indicated that successfully completing the operation should not
 * generate a completion queue event (AKA 'detached operations'). In
 * these cases, a failure of the operation can be ignored (or perhaps
 * logged) when handling the event, and the this value can be used to
 * perform such filtering. This reserved value is represented by the
 * kDetachedOperationFailure constant
 */

// TODO(bd) remove debugging output once the code is confirmed stable
// #define JMG_ENABLE_REACTOR_DEBUGGING_OUTPUT

#if defined(JMG_ENABLE_REACTOR_DEBUGGING_OUTPUT)
#define JMG_URING_LOG_DEBUG(ring, ...) ring->debugLog(__VA_ARGS__)
#else
#define JMG_URING_LOG_DEBUG(ring, ...)
#endif

namespace jmg::uring
{

////////////////////////////////////////////////////////////////////////////////
// safe types for uring
////////////////////////////////////////////////////////////////////////////////

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using UringSz = SafeType<uint32_t>;
using UringFlags = SafeType<uint32_t>;
using UserData = SafeType<uint64_t, st::equality_comparable>;
using ListenBacklog = SafeType<int>;
#else
JMG_NEW_SIMPLE_SAFE_TYPE(UringSz, uint32_t);
JMG_NEW_SIMPLE_SAFE_TYPE(UringFlags, uint32_t);
JMG_NEW_SAFE_TYPE(UserData, uint64_t, st::equality_comparable);
JMG_NEW_SIMPLE_SAFE_TYPE(ListenBacklog, int);
#endif

using IoVecView = std::span<const struct iovec>;

////////////////////////////////////////////////////////////////////////////////
// constants for uring
////////////////////////////////////////////////////////////////////////////////

constexpr auto kDetachedOperationFailure = UserData(-1);

constexpr auto kDefaultUringFlags = UringFlags(0);

constexpr auto kMaxListenQueueBacklog = ListenBacklog(SOMAXCONN);

constexpr auto kDefaultListenQueueBacklog = ListenBacklog(0);

/**
 * indication of whether the submission of requests to the io_uring
 * should be delayed to allow multiple operations, possibly linked, to
 * be submitted as efficiently as possible
 */
enum class DelaySubmission : bool { kDelay = true, kNoDelay = false };

/**
 * indication of side of socket to retrieve socket info from when
 * getting socket info
 */
enum class SocketSide : int32_t { kLocal = 0, kPeer = 1 };

namespace detail
{
template<typename T>
concept CommonExtraT =
  SameAsDecayedT<DelaySubmission, T> || SameAsDecayedT<UserData, T>;
} // namespace detail

/**
 * wrapper around the CQE result received from waiting for a uring event
 *
 * @note destroying the object will automatically mark the CQE as consumed,
 * unless this action has been explicitly canceled
 */
class Event {
public:
  Event() = default;
  JMG_NON_COPYABLE(Event);
  // move operations are defined explicitly
  Event(Event&& src);
  Event& operator=(Event&& src);

  Event(io_uring* ring, io_uring_cqe* cqe);

  ~Event();

  operator bool() const { return pred(cqe_); }

  const io_uring_cqe& operator*() const {
    JMG_ENFORCE_USING(std::logic_error, pred(cqe_),
                      "attempted to retrieve CQE reference that was not set");
    return *cqe_;
  }

  const io_uring_cqe* operator->() const { return cqe_; }

  UserData getUserData() const {
    // NOTE: using operator* to avoid SIGSEGV if the CQE pointer is not set
    return UserData(cqe_->user_data);
  }

private:
  io_uring* ring_ = nullptr;
  io_uring_cqe* cqe_ = nullptr;
};

/**
 * wrapper around an io_uring object that improves usability
 */
class Uring {
public:
  Uring() = delete;
  JMG_NON_COPYABLE(Uring);
  JMG_NON_MOVEABLE(Uring);
  ~Uring();

  // TODO(bd) allow user to pass flags as well?
  /**
   * @param sz - number of entries in the uring circular queue
   */
  explicit Uring(const UringSz sz);

  ////////////////////////////////////////////////////////////
  // uring event management

  /**
   * call this after adding one or more requests to uring with delayed submission
   */
  void submitReq(const std::string_view req_type) {
    JMG_SYSTEM(io_uring_submit(&ring_), "unable to submit io_uring ", req_type,
               " request");
    // NOTE: intentionally ignoring the submission count to allow multiple
    // submissions to be chained
  }

  /**
   * fast poll of the ring in user space to check if some event is
   * available
   */
  bool hasEvent();

  /**
   * wait for the next uring event to occur, or until the timeout, if any
   */
  Event awaitEvent(std::optional<Duration> timeout = std::nullopt);

  ////////////////////////////////////////////////////////////
  // inter-uring communication

  /**
   * external threads that have access to a separate io_uring can use this file
   * descriptor to send messages to this uring
   */
  FileDescriptor getChannel() const { return channel_; }

  ////////////////////////////////////////////////////////////
  // inter-fiber communication

  /**
   * register an event fd to be used as a notification mechanism
   */
  void registerEventNotifier(
    EventFd notifier,
    DelaySubmission isDelayed = DelaySubmission::kNoDelay);

  ////////////////////////////////////////////////////////////
  // logging

  /**
   * log debug data to stdout using a detached operation that will no
   * produce a completion event if it is successful
   */
  template<typename... Args>
  void debugLog(Args&&... args) {
    // NOTE: locally constructed message must be stored until it has
    // been consumed by the ring, but there is probably a better way
    // to do this...
    debug_msgs_.emplace_front(str_cat(">>>>> DBG ", std::forward<Args>(args)...,
                                      "\n"));
    submitDebugLogReq(iov_from(debug_msgs_.front()));
  }

  ////////////////////////////////////////////////////////////
  // misc utilities

  /**
   * submit a request to close an open descriptor
   */
  template<DescriptorT T, detail::CommonExtraT... Extras>
  void submitFdCloseReq(const T fd, Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitFdCloseReq(unsafe(fd), is_delayed, user_data);
  }

  /**
   * submit a request for a timeout event to occur in the future
   *
   * @note this is separate from the timeout that can occur when waiting for the
   * uring to receive events, it is intended to allow external threads to
   * schedule timeout events directly with the kernel in order to avoid the need
   * to maintain such a schedule internally e.g. with a priority queue
   */
  template<typename T>
    requires TimePointT<T> || DurationT<T>
  void submitTimerEventReq(T&& timeout, UserData user_data) {
    UringTimeSpec ts;
    unsigned flags = 0;
    if constexpr (TimePointT<T>) {
      flags = IORING_TIMEOUT_ABS;
      // convert TimePoint but then copy the values over to
      // UringTimeSpec
      const ::timespec ts_tp = from(timeout);
      ts.tv_sec = ts_tp.tv_sec;
      ts.tv_nsec = ts_tp.tv_nsec;
    }
    else { ts = from(timeout); }
    submitTimeoutReq(ts, flags, user_data);
  }

  ////////////////////////////////////////////////////////////
  // file support

  /**
   * submit a request to open a file
   */
  template<detail::CommonExtraT... Extras>
  void submitFileOpenReq(c_string_view file_path,
                         FileOpenFlags flags,
                         mode_t permissions,
                         Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitFileOpenReq(file_path, flags, permissions, is_delayed, user_data);
  }

  ////////////////////////////////////////////////////////////
  // networking support

  /**
   * submit a request to open a socket
   */
  template<detail::CommonExtraT... Extras>
  void submitSocketOpenReq(SocketTypes socket_type, Extras&&... extras) {
    using namespace std::string_view_literals;
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    auto& sqe = getNextSqe();
    switch (socket_type) {
      case SocketTypes::kTcp:
        io_uring_prep_socket(&sqe, AF_INET, SOCK_STREAM, 0, 0);
        break;
      case SocketTypes::kUdp:
        break;
      default:
        JMG_THROW_EXCEPTION(std::logic_error, "unknown socket type [",
                            static_cast<int>(socket_type), "]");
    }
    reqFinalize(sqe, is_delayed, user_data, "open socket"sv);
  }

  /**
   * submit a request to form a network connection to an endpoint
   */
  template<detail::CommonExtraT... Extras>
  void submitNetConnectReq(const SocketDescriptor sd,
                           const IpEndpoint& tgt_endpoint,
                           Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitNetConnectReq(unsafe(sd), tgt_endpoint, is_delayed, user_data);
  }

  /**
   * submit a request to receive data from a socket, which will be
   * stored in a referenced buffer object
   */
  template<detail::CommonExtraT... Extras>
  void submitRecvFromReq(const SocketDescriptor sd,
                         const BufferProxy buf,
                         const int flags,
                         Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitRecvFromReq(unsafe(sd), buf, flags, is_delayed, user_data);
  }

  /**
   * submit a request to receive data from a socket, which will be
   * stored in a referenced buffer object
   */
  template<detail::CommonExtraT... Extras>
  void submitSetSockOptReq(const SocketDescriptor sd,
                           const int level,
                           const int opt_name,
                           const void* opt_val,
                           const size_t opt_sz,
                           Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitSetSockOptReq(unsafe(sd), level, opt_name, opt_val, opt_sz,
                        is_delayed, user_data);
  }

  /**
   * submit a request to bind an existing socket to an endpoint
   *
   * TODO(bd) support more optionality in protocols and interfaces
   */
  template<typename... Extras>
  // TODO(bd) add 'requires' statement here
  void submitSocketBindReq(const SocketDescriptor sd, Extras&&... extras) {
    // TODO(bd) figure out how to support some version of
    // getCommonExtras here
    auto port = Port(0);
    DelaySubmission is_delayed = DelaySubmission::kNoDelay;
    std::optional<UserData> user_data = std::nullopt;
    [[maybe_unused]]
    auto processParam = [&]<typename T>(const T arg) {
      if constexpr (jmg::DecayedSameAsT<T, DelaySubmission>) {
        is_delayed = arg;
      }
      else if constexpr (jmg::DecayedSameAsT<T, UserData>) { user_data = arg; }
      else if constexpr (jmg::DecayedSameAsT<T, Port>) { port = arg; }
    };
    (processParam(std::forward<Extras>(extras)), ...);
    submitSocketBindReq(unsafe(sd), port, is_delayed, user_data);
  }

  /**
   * submit a request to prepare an existing socket to accept
   * connections
   */
  template<typename... Extras>
  // TODO(bd) add 'requires' statement here
  void submitSocketListenReq(const SocketDescriptor sd, Extras&&... extras) {
    // TODO(bd) figure out how to support some version of
    // getCommonExtras here
    auto backlog = kDefaultListenQueueBacklog;
    DelaySubmission is_delayed = DelaySubmission::kNoDelay;
    std::optional<UserData> user_data = std::nullopt;
    [[maybe_unused]]
    auto processParam = [&]<typename T>(const T arg) {
      if constexpr (jmg::SameAsDecayedT<T, DelaySubmission>) {
        is_delayed = arg;
      }
      else if constexpr (jmg::SameAsDecayedT<T, UserData>) { user_data = arg; }
      else if constexpr (jmg::SameAsDecayedT<T, ListenBacklog>) {
        backlog = arg;
      }
    };
    (processParam(std::forward<Extras>(extras)), ...);
    submitSocketListenReq(unsafe(sd), unsafe(backlog), is_delayed, user_data);
  }

#if defined(JMG_LIBURING_VERSION_SUPPORTS_GETSOCKNAME)
  template<detail::CommonExtraT... Extras>
  void submitSocketInfoReq(const SocketDescriptor sd,
                           struct sockaddr& addr,
                           socklen_t& addrSz,
                           SocketSide side,
                           Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitSocketInfoReq(unsafe(sd), addr, addrSz, side, is_delayed, user_data);
  }
#endif

  ////////////////////////////////////////////////////////////
  // reading and writing data

  /**
   * submit a request for data, referenced via a view into an iovec object, to
   * be written to a file descriptor
   */
  template<WritableDescriptorT Fd, typename... Extras>
  void submitWriteReq(Fd fd, IoVecView io_vec, Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitWriteReq(unsafe(fd), io_vec, is_delayed, user_data);
  }

  /**
   * submit a request for data, whose 'shape' is determined by an iovec object
   * referenced by a view, to be read from a file descriptor
   */
  template<ReadableDescriptorT Fd, typename... Extras>
  void submitReadReq(Fd fd, IoVecView io_vec, Extras&&... extras) {
    const auto [is_delayed, user_data] =
      getCommonExtras(std::forward<Extras>(extras)...);
    submitReadReq(unsafe(fd), io_vec, is_delayed, user_data);
  }

private:
  /**
   * helper function template that retrieves commonly used parameters
   * for indicating whether submission should be delayed and for
   * providing user data
   */
  template<detail::CommonExtraT... Extras>
  auto getCommonExtras(Extras&&... extras) {
    DelaySubmission is_delayed = DelaySubmission::kNoDelay;
    std::optional<UserData> user_data = std::nullopt;
    [[maybe_unused]]
    auto processParam = [&]<typename T>(const T arg) {
      if constexpr (jmg::DecayedSameAsT<T, DelaySubmission>) {
        is_delayed = arg;
      }
      else if constexpr (jmg::DecayedSameAsT<T, UserData>) { user_data = arg; }
      // TODO(bd) check for non-exhaustive case analysis?
    };
    (processParam(std::forward<Extras>(extras)), ...);
    return std::make_tuple(is_delayed, user_data);
  }

  /**
   * get next available submission queue entry
   */
  io_uring_sqe& getNextSqe();

  /**
   * helper function that handles the common behavior of completing a
   * uring request submission
   */
  void reqFinalize(io_uring_sqe& sqe,
                   DelaySubmission is_delayed,
                   std::optional<UserData> user_data,
                   std::string_view req_type);

  /**
   * implementation of close request submission
   */
  void submitFdCloseReq(int fd,
                        DelaySubmission is_delayed,
                        std::optional<UserData> user_data);

  /**
   * implementation of timeout request submission
   */
  void submitTimeoutReq(UringTimeSpec timeout_spec,
                        unsigned flags,
                        UserData user_data);

  /**
   * implementation of file open request submission
   */
  void submitFileOpenReq(c_string_view file_path,
                         FileOpenFlags flags,
                         mode_t permissions,
                         DelaySubmission is_delayed,
                         std::optional<UserData> user_data);

  /**
   * implementation of network connection request submission
   */
  void submitNetConnectReq(int sd,
                           const IpEndpoint& tgt_endpoint,
                           DelaySubmission is_delayed,
                           std::optional<UserData> user_data);
  /**
   * implementation of write request submission
   */
  void submitWriteReq(int fd,
                      IoVecView io_vec,
                      DelaySubmission is_delayed,
                      std::optional<UserData> user_data);

  /**
   * implementation of read request submission
   */
  void submitReadReq(int fd,
                     IoVecView io_vec,
                     DelaySubmission is_delayed,
                     std::optional<UserData> user_data);

  /**
   * implementation of recv request submission
   */
  void submitRecvFromReq(int sd,
                         BufferProxy buf,
                         int flags,
                         DelaySubmission is_delayed,
                         std::optional<UserData> user_data);

  /**
   * implementation of setsockopt request submission
   *
   * TODO(bd) support IPv6
   */
  void submitSetSockOptReq(int sd,
                           int level,
                           int opt_name,
                           const void* opt_val,
                           size_t opt_sz,
                           DelaySubmission is_delayed,
                           std::optional<UserData> user_data);

  /**
   * implementation of bind request submission
   */
  void submitSocketBindReq(int sd,
                           Port port,
                           DelaySubmission is_delayed,
                           std::optional<UserData> user_data);

  /**
   * implementation of listen request submission
   */
  void submitSocketListenReq(int sd,
                             int backlog,
                             DelaySubmission is_delayed,
                             std::optional<UserData> user_data);

#if defined(JMG_LIBURING_VERSION_SUPPORTS_GETSOCKNAME)
  /**
   * implementation of getsockname request submission
   */
  void submitSocketInfoReq(int sd,
                           struct sockaddr& addr,
                           socklen_t& addrSz,
                           SocketSide side,
                           DelaySubmission is_delayed,
                           std::optional<UserData> user_data);
#endif

  /**
   * implementation of debug log request submission
   */
  void submitDebugLogReq(IoVecView io_vec);

  std::optional<EventFd> notifier_;
  io_uring ring_;
  std::atomic<FileDescriptor> channel_ = kInvalidFileDescriptor;
  std::forward_list<std::string> debug_msgs_;
};

} // namespace jmg::uring
