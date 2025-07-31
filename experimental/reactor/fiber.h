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

#include <ucontext.h>

#include "control_blocks.h"

namespace jmg
{

class Reactor;

enum class FiberState : uint8_t {
  kUnallocated = 0,
  kActive, // NOTE: only one thread at a time should be active
  kBlocked,
  kRunnable,
  kTerminated
};

using FiberId = CtrlBlockId;

/**
 * publicly accessible interface to a reactor fiber object
 */
class Fiber {
public:
  Fiber() = default;
  JMG_NON_COPYABLE(Fiber);
  JMG_NON_MOVEABLE(Fiber);
  ~Fiber() = default;

  FiberId getId() const { return id_; }

  void yield();

private:
  friend class Reactor;

  Fiber(FiberId id, Reactor& reactor) : id_(id), reactor_(&reactor) {}

  FiberId id_;
  Reactor* reactor_ = nullptr;
};

using FiberFcn = std::function<void(Fiber&)>;

// TODO(bd) support variable size segmented stacks
static constexpr size_t kStackSz = 16384;
struct FiberCtrlBlockBody {
  ucontext_t chkpt;
  std::array<uint8_t, kStackSz> stack;
  FiberState state;
  Fiber fbr;
  bool is_fiber_yielding = false;
  std::unique_ptr<FiberFcn> fbr_fcn;
};
using FiberCtrl = ControlBlocks<FiberCtrlBlockBody>;
using FiberCtrlBlock = FiberCtrl::ControlBlock;
using FiberCtrlBlockQueue = CtrlBlockQueue<FiberCtrlBlockBody>;

} // namespace jmg
