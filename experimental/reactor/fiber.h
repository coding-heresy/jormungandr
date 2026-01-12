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

#include <array>
#include <filesystem>

#include <ucontext.h>

#include "control_blocks.h"
#include "uring.h"

namespace jmg
{

class Reactor;

// TODO(bd) add a 'kYielding' state?
enum class FiberState : uint8_t {
  kUnallocated = 0,
  kActive, // NOTE: only one thread at a time should be active
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
  using Uring = uring::Uring;
  using UserData = uring::UserData;

public:
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

  /**
   * write output to stdout
   */
  template<typename... Args>
  void log(Args&&... args) {
    const auto output = str_cat(std::forward<Args&&>(args)...);
    write(kStdoutFd, buffer_from(output));
  }

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

#if defined(TMP_THREAD_POOL_WORKS)
  ////////////////////
  // thread pool execution support

  template<typename Fcn>
    requires std::invocable<Fcn>
  void execute(Fcn&& fcn) {
    reactor_->execute(std::forward<Fcn>(fcn));
  }
#endif

  ////////////////////
  // file support

  /**
   * open a file
   */
  FileDescriptor openFile(const std::filesystem::path& filePath,
                          FileOpenFlags flags,
                          std::optional<mode_t> permissions = std::nullopt);

  ////////////////////
  // networking support

  /**
   * open a socket
   */
  SocketDescriptor openSocket(SocketTypes socket_type);

  /**
   * connect to a (possibly remote) network endpoint
   */
  void connectTo(SocketDescriptor sd, const IpEndpoint& tgt_endpoint);

  ////////////////////
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
  void write(T fd, BufferView data) {
    if (data.empty()) { return; }
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
    validateEvent("write data");
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
  Event getEvent(const std::string_view op);

  /**
   * perform sanity checks on the outstanding Event object associated with the
   * fiber, but do not return it for processing by the caller
   */
  void validateEvent(const std::string_view op);

  FiberId id_;
  Reactor* reactor_ = nullptr;
  Uring* uring_ = nullptr;
  FiberCtrlBlockBody* fcb_body_ = nullptr;
};

using FiberFcn = std::function<void(Fiber&)>;

// TODO(bd) support variable size segmented stacks
static constexpr size_t kStackSz = 16384;

struct FiberCtrlBlockBody {
  ucontext_t chkpt;
  std::array<uint8_t, kStackSz> stack;
  Fiber fbr;
  std::unique_ptr<FiberFcn> fbr_fcn;
  uring::Event event;
  // TODO(bd) is it really necessary for these variables to be volatile?
  volatile FiberState state;
  // TODO(bd) these flags can probably be converted to specific state values
  volatile bool is_fiber_yielding = false;
  volatile bool is_fiber_handling_event = false;
};
using FiberCtrl = ControlBlocks<FiberCtrlBlockBody>;
using FiberCtrlBlock = FiberCtrl::ControlBlock;
using FiberCtrlBlockQueue = CtrlBlockQueue<FiberCtrlBlockBody>;

} // namespace jmg
