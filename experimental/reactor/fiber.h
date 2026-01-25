/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ucontext.h>

#include <array>
#include <filesystem>
#include <vector>

#include "jmg/ip_endpoint.h"
#include "jmg/types.h"

#include "jmg/reactor/control_blocks.h"
#include "jmg/reactor/uring.h"

namespace jmg
{

class Reactor;

// TODO(bd) add a 'kYielding' state?
enum class FiberState : uint8_t {
  kUnallocated = 0,
  kEmbryonic,
  kActive, // NOTE: only one thread at a time should be active
  kYielding,
  kBlocked,
  kRunnable,
  kTerminated
};

using FiberId = CtrlBlockId;

struct FiberCtrlBlockBody;

/**
 * publicly accessible interface to a reactor fiber object
 */
class Fiber {
  using DelaySubmission = uring::DelaySubmission;
  using Event = uring::Event;
  using FiberFcn = std::function<void(Fiber&)>;
  using Uring = uring::Uring;
  using UserData = uring::UserData;
  using WorkerFcn = std::function<void(void)>;

public:
  using IpEndpoints = std::vector<IpEndpoint>;

  Fiber() = default;
  JMG_NON_COPYABLE(Fiber);
  JMG_NON_MOVEABLE(Fiber);
  ~Fiber() = default;

  /**
   * get the fiber ID associated with the fiber object
   */
  FiberId getId() const { return id_; }

  /**
   * explicitly yield execution to other currently runnable fibers
   */
  void yield();

  ////////////////////////////////////////////////////////////
  // thread pool execution support

  // TODO(bd) add support for std::move_only_function?
  /**
   * send a task to the thread pool associated with the reactor without
   * expecting a result
   */
  void execute(WorkerFcn fcn);

  /**
   * send a computation task to the thread pool associated with the
   * reactor and return the resulting value back to the fiber
   *
   * NOTE: this function will automatically capture any exceptions
   * thrown within the body of its compute function and propagate them
   * to the caller
   */
  template<typename Fcn, typename... Args>
    requires std::invocable<Fcn, Args...>
  auto compute(Fcn&& fcn, Args&&... args) {
    using Rslt = std::invoke_result_t<Fcn, Args...>;
    Rslt rslt;
    std::exception_ptr exc_ptr;
    execute([&rslt, &exc_ptr, notify_fbr = makeFbrNotifier(getId()),
             fcn = std::forward<Fcn>(fcn),
             ... args = std::forward<Args>(args)]() mutable {
      // using a thread pool worker, execute the function and assign
      // the return value back to the result object or propagate any
      // exception that it may throw
      try {
        rslt = std::invoke(std::forward<Fcn>(fcn), std::forward<Args>(args)...);
      }
      catch (...) {
        exc_ptr = std::current_exception();
      }
      notify_fbr();
    });

    reschedule();

    // propagate any exception thrown in the function
    if (exc_ptr) { std::rethrow_exception(exc_ptr); }

    // return the result if there was no exception
    return rslt;
  }

  ////////////////////////////////////////////////////////////
  // logging

  /**
   * write output to stdout
   */
  template<typename... Args>
  void log(Args&&... args) {
    const auto output =
      str_cat("[", id_, "] ", std::forward<Args&&>(args)..., "\n");
    write(kStdoutFd, buffer_from(output));
  }

  ////////////////////////////////////////////////////////////
  // misc functions

  /**
   * close an open descriptor of any kind
   */
  template<DescriptorT T>
  void close(T fd) {
    // set the user data to the fiber ID so the completion event gets routed
    // back to this thread
    uring_->submitFdCloseReq(fd, UserData(unsafe(id_)));

    ////////////////////
    // enter the scheduler to defer further processing until the operation is
    // complete
    reschedule();

    ////////////////////
    // return from scheduler
    validateEvent("close descriptor");
  }

  template<typename T>
    requires TimePointT<T> || DurationT<T>
  void awaitTimeout(T&& timeout) {
    if constexpr (TimePointT<T>) {
      // timeout is at an absolute time point
      TimePoint timeout_tp = from(timeout);
      auto ts = getCurrentTime();
      JMG_ENFORCE(timeout_tp > ts, "provided timeout time [",
                  jmg::to_string(timeout_tp),
                  "] was earlier than or the same as current time [",
                  jmg::to_string(ts), "]");
      uring_->submitTimerEventReq(timeout_tp, UserData(unsafe(id_)));
    }
    else {
      // timeout occurs after a time duration
      Duration timeout_duration = from(timeout);
      uring_->submitTimerEventReq(timeout_duration, UserData(unsafe(id_)));
    }

    ////////////////////
    // enter the scheduler to defer further processing until the operation is
    // complete
    reschedule();

    ////////////////////
    // return from scheduler
    getEvent("await timeout", true /* is_timer */);
  }

  /**
   * create a new fiber that will execute the argument function
   */
  void spawn(FiberFcn&& fcn);

  ////////////////////////////////////////////////////////////
  // file support

  /**
   * open a file
   */
  FileDescriptor openFile(const std::filesystem::path& filePath,
                          FileOpenFlags flags,
                          std::optional<mode_t> permissions = std::nullopt);

  ////////////////////////////////////////////////////////////
  // networking support

  /**
   * open a socket
   */
  SocketDescriptor openSocket(SocketTypes socket_type = SocketTypes::kTcp);

  /**
   * connect to a (possibly remote) network endpoint
   */
  void connectTo(SocketDescriptor sd, const IpEndpoint& tgt_endpoint);

  /**
   * read data from a socket
   *
   * TODO(bd) create a safe type for flags
   */
  size_t recvFrom(SocketDescriptor fd, BufferProxy buf, int flags);

  /**
   * set options for a socket
   *
   * TODO(bd) support multiple options in a single command
   *
   * TODO(bd) create safe types for level and opt_id
   *
   * TODO(bd) create an owning type for the combination of level,
   * opt_id, opt_val and opt_sz
   */
  void setSocketOption(SocketDescriptor sd,
                       int level,
                       int opt_id,
                       const void* opt_val,
                       size_t opt_sz);

  /**
   * bind a socket to a network interface and (optionally) protocol
   * port on the local host for later use in listening for connections
   */
  void bindSocketToIfce(SocketDescriptor sd, IpPort port);

  /**
   * enable connection listening on a socket that was previously bound
   * to a local interface and port
   */
  void enableListenSocket(SocketDescriptor sd,
                          std::optional<uring::ListenBacklog> backlog =
                            uring::kDefaultListenQueueBacklog);

  /**
   * accept a connection on a socket that was previously enabled to
   * listen returning the new socket descriptor associated with the
   * connection
   */
  std::tuple<SocketDescriptor, IpEndpoint> acceptCnxn(SocketDescriptor sd);

  /**
   * lookup the list of IP endpoints associated with a host
   */
  template<NullTerminatedStringT Str, typename... Args>
  IpEndpoints lookupNetworkEndpoints(const Str& host, Args&&... args) {
    c_string_view svc;
    [[maybe_unused]]
    auto processArg = [&]<typename Arg>(Arg&& arg) {
      if constexpr (NullTerminatedStringT<Arg>) { svc = c_string_view(arg); }
      // TODO(bd) handle multiple protocols along with flags and
      // timeouts
      else { JMG_NOT_EXHAUSTIVE(Arg); }
    };
    (processArg(args), ...);
    // delegate to the reactor thread pool because there doesn't seem
    // to be a fully functional solution that is compatible with
    // io_uring
    return compute([&] mutable -> IpEndpoints {
      const auto* host_ptr = c_string_view(host).c_str();
      const auto* svc_ptr = svc.empty() ? nullptr : svc.c_str();
      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      // TODO(bd) support IPv6 (via AF_INET6)
      hints.ai_family = AF_INET;
      // TODO(bd) support UDP
      hints.ai_socktype = SOCK_STREAM;

      struct addrinfo* info_ptr = nullptr;
      const auto rc = ::getaddrinfo(host_ptr, svc_ptr, &hints, &info_ptr);
      if (rc != 0) {
        JMG_THROW_EXCEPTION(std::runtime_error,
                            "unable to lookup network endpoints: ",
                            ::gai_strerror(rc));
      }
      struct addrinfo* ptr = nullptr;
      size_t rslt_sz = 0;
      for (ptr = info_ptr; ptr; ptr = ptr->ai_next) {
        if (AF_INET == ptr->ai_family) { ++rslt_sz; }
        // TODO(bd) support IPv6 (via AF_INET6)
      }
      IpEndpoints rslt;
      if (!rslt_sz) { return rslt; }
      rslt.reserve(rslt_sz);
      for (ptr = info_ptr; ptr; ptr = ptr->ai_next) {
        if (AF_INET == ptr->ai_family) { rslt.emplace_back(*(ptr->ai_addr)); }
      }
      return rslt;
    });
  }

  ////////////////////////////////////////////////////////////
  // reading and writing data

  /**
   * read data from an open file descriptor
   */
  template<ReadableDescriptorT T>
  size_t read(T fd, BufferProxy buf) {
    auto iov = iov_from(buf);
    // set the user data to the fiber ID so the completion event gets routed
    // back to this thread
    uring_->submitReadReq(fd, iov, DelaySubmission::kNoDelay,
                          UserData(unsafe(id_)));

    ////////////////////
    // enter the scheduler to defer further processing until the operation is
    // complete
    reschedule();

    ////////////////////
    // return from scheduler
    const auto event = getEvent("read data");
    const auto& cqe = *(event);
    return static_cast<size_t>(cqe.res);
  }

  /**
   * write data to an open file descriptor
   */
  template<WritableDescriptorT T>
  size_t write(T fd, BufferView data) {
    if (data.empty()) { return 0; }
    auto iov = iov_from(data);
    // set the user data to the fiber ID so the completion event gets routed
    // back to this thread
    uring_->submitWriteReq(fd, iov, DelaySubmission::kNoDelay,
                           UserData(unsafe(id_)));

    ////////////////////
    // enter the scheduler to defer further processing until the operation is
    // complete
    reschedule();

    ////////////////////
    // return from scheduler
    const auto event = getEvent("write data");
    const auto& cqe = *(event);
    return static_cast<size_t>(cqe.res);
  }

private:
  friend class Reactor;

  Fiber(FiberId id, Reactor& reactor);

  /**
   * non-blocking close of generic (file) descriptor
   */
  void close(int fd);

  /**
   * execute the reactor scheduler to block the current fiber until a requested
   * action (or actions) is complete
   */
  void reschedule();

  /**
   * get the outstanding Event object associated with the fiber
   *
   * also performs several sanity checks
   */
  Event getEvent(const std::string_view op, bool is_timer = false);

  /**
   * perform sanity checks on the outstanding Event object associated with the
   * fiber, but do not return it for processing by the caller
   */
  void validateEvent(const std::string_view op);

  /**
   * construct a function that external code can use to notify some
   * fiber of available data
   *
   * @warning the notifier object contains a non-owning reference to
   * the reactor and should thus only be passed to reactor thread pool
   * workers, whose lifetimes are guaranteed not to exceed that of the
   * reactor itself by construction
   */
  WorkerFcn makeFbrNotifier(FiberId id);

  FiberId id_;
  Reactor* reactor_ = nullptr;
  Uring* uring_ = nullptr;
  FiberCtrlBlockBody* fcb_body_ = nullptr;
};

using FiberFcn = std::function<void(Fiber&)>;

struct FiberCtrlBlockBody {
private:
  // TODO(bd) support variable size segmented stacks
  static constexpr size_t kStackSz = 16384;

public:
  ucontext_t chkpt;
  std::array<uint8_t, kStackSz> stack;
  Fiber fbr;
  std::unique_ptr<FiberFcn> fbr_fcn;
  uring::Event event;
  // TODO(bd) is it really necessary for these variables to be volatile?
  volatile FiberState state;
};
using FiberCtrl = ControlBlocks<FiberCtrlBlockBody>;
using FiberCtrlBlock = FiberCtrl::ControlBlock;
using FiberCtrlBlockQueue = CtrlBlockQueue<FiberCtrlBlockBody>;

} // namespace jmg
