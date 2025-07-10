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
#include "uring.h"

namespace jmg
{

/**
 * commands that can be sent to the reactor thread from external threads
 */
enum class Cmd : uint64_t {
  // NOTE: initial value is 1 because writing a 0 to an eventfd will cause a
  // read against it to produce an EAGAIN/EWOULDBLOCK error
  kShutdown = 1,
  kPost = 2
};

class Reactor {
public:
  /**
   * start the reactor, takes control of the thread that calls it
   */
  void start();

  /**
   * instruct the reactor to shut down, should not be called from fiber code
   * executing in the reactor.
   */
  void shutdown();

private:
  using OptMillisec = std::optional<std::chrono::milliseconds>;
  using OptStrView = std::optional<std::string_view>;

  /**
   * pass control from a fiber to the reactor scheduler when the fiber executes
   * an async operation
   */
  void schedule(const OptMillisec timeout = std::nullopt);

  /**
   * perform some sketchy shenanigans to a allow a call to setcontext to execute
   * a capturing lambda
   */
  static void trampoline(const intptr_t lambda_ptr_val);

  /**
   * store the current checkpoint directly into a context buffer
   */
  static void saveChkpt(ucontext_t& chkpt,
                        const OptStrView operation = std::nullopt);

  /**
   * store the current checkpoint into a fiber control block
   */
  static void saveChkpt(FiberCtrlBlock& fcb,
                        const OptStrView operation = std::nullopt) {
    saveChkpt(fcb.body.chkpt, operation);
  }

  /**
   * jump to the checkpoint stored in a fiber control block
   *
   * @warning the actual call to setcontext should never be placed in a separate
   * function inside this one because it will smash the stack for some reason
   */
  [[noreturn]] void jumpTo(FiberCtrlBlock& fcb,
                           const OptStrView tgt = std::nullopt);

  /**
   * construct a context that can execute a capturing lamda
   */
  void initFbr(FiberCtrlBlock& fcb,
               FiberFcn& fcn,
               const OptStrView operation = std::nullopt,
               ucontext_t* returnTgt = nullptr);

  FiberId active_fiber_id_;
  FiberCtrl fiber_ctrl_;
  ucontext_t shutdown_chkpt_;
  std::optional<EventFd> notifier_;
  std::unique_ptr<uring::Uring> uring_;
  std::atomic<bool> is_shutdown_ = false;
};

} // namespace jmg
