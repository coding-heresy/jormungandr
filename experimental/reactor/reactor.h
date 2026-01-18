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

#include <span>

#include "jmg/preprocessor.h"
#include "jmg/types.h"
#include "jmg/util.h"

#include "control_blocks.h"
#include "fiber.h"
#include "thread_pool.h"
#include "uring.h"

namespace jmg
{

namespace detail
{
void fiberTrampoline(const intptr_t reactor_ptr_val, const intptr_t fbr_id_val);
} // namespace detail

class Reactor {
public:
  // TODO(bd) uring size should be at settable at compile or run time
  Reactor(size_t thread_pool_worker_count = 1);
  ~Reactor();
  JMG_NON_COPYABLE(Reactor);
  JMG_NON_MOVEABLE(Reactor);

  /**
   * start the reactor, takes control of the thread that calls it
   */
  void start();

  /**
   * instruct the reactor to shut down, should not be called from fiber code
   * executing in the reactor.
   */
  void shutdown();

  /**
   * send work to be performed by a new fiber in the reactor
   */
  void execute(FiberFcn&& fcn);

private:
  static constexpr size_t kMaxFibers = FiberCtrl::kMaxFibers;
  static constexpr uint64_t kShutdownCmd = std::numeric_limits<uint64_t>::max();

  using OptMillisec = std::optional<std::chrono::milliseconds>;
  using OptStrView = std::optional<std::string_view>;
  using WorkerFcn = Fiber::WorkerFcn;

  // TODO(bd) maybe experiment with boost thread pool once it is
  // building correctly
  // using ThreadPool = BoostThreadPool;

  // TODO(bd) maybe experiment with BsThreadPool
  // using ThreadPool = BsThreadPool;

  // NOTE: currently using DpThreadPool
  using ThreadPool = DpThreadPool;

  friend class Fiber;
  friend void detail::fiberTrampoline(const intptr_t reactor_ptr_val,
                                      const intptr_t fbr_id_val);

  /**
   * pass control from a fiber to the reactor scheduler when the fiber executes
   * an async operation
   */
  void schedule(const OptMillisec timeout = std::nullopt);

  /**
   * jump to the checkpoint stored in a fiber control block
   */
  void jumpTo(FiberCtrlBlock& fcb, const OptStrView tgt = std::nullopt);

  /**
   * initialize a fiber that will execute a thread worker function
   */
  FiberCtrlBlock& initFbr(WorkerFcn& fcn,
                          const OptStrView operation = std::nullopt,
                          ucontext_t* return_tgt = nullptr);

  /**
   * initialize a fiber that will execute a fiber function
   */
  FiberCtrlBlock& initFbr(FiberFcn&& fcn,
                          const OptStrView operation = std::nullopt,
                          // TODO(bd) is this necessary?
                          ucontext_t* return_tgt = nullptr);

  /**
   * get the active fiber it, implemented as a function to deal with annoying
   * issues related to the 'volatile' qualifier
   */
  FiberId getActiveFbrId() const noexcept {
    return *((const FiberId*)(&active_fiber_id_));
  }

  /**
   * set the active fiber it, implemented as a function to deal with annoying
   * issues related to the 'volatile' qualifier
   */
  void setActiveFbrId(const FiberId id) noexcept {
    (FiberId&)active_fiber_id_ = id;
  }

  /**
   * yield the currently active fiber
   */
  void yieldFbr();

  /**
   * reset the notifier mechanism used by external threads to post work to the
   * reactor after a work request has been read
   */
  void resetNotifier();

  /**
   * send work to be executed on the thread pool without waiting for
   * results
   */
  template<typename Fcn>
    requires std::invocable<Fcn>
  void execute(Fcn&& fcn) {
    thread_pool_.execute(std::move(fcn));
  }

  /**
   * notify a fiber that thread pool computation data is ready for
   * consumption
   */
  void notifyFbr(FiberId id);

  std::atomic<bool> is_shutdown_ = false;
  FiberCtrl fiber_ctrl_;
  std::unique_ptr<uring::Uring> uring_;
  ucontext_t shutdown_chkpt_;

  FiberCtrlBlockQueue runnable_;
  ThreadPool thread_pool_;

  // NOTE: active_fiber_id_ is marked as volatile because it can change when
  // jumping away from a context and then resuming it
  volatile FiberId active_fiber_id_;

  // TODO(bd) there is almost certainly a better/faster way to support
  // having external threads send work requests to the reactor, but
  // this will do for now
  //
  // notifier mechanism below here, which consists of an old-school
  // pipe whose read fd is used to receive requests and write fd is
  // used to send requests from external threads by writing the
  // address of a function object to be executed
  PipeReadFd post_tgt_;
  PipeWriteFd post_src_;
  uint64_t notifier_data_;
  struct iovec notifier_vec_[1];
};

} // namespace jmg
