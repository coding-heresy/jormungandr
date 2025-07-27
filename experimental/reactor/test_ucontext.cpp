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

#include <ucontext.h>

#include <array>
#include <atomic>
#include <memory>
#include <string_view>
#include <tuple>

#include <gtest/gtest.h>

#include "jmg/preprocessor.h"
#include "jmg/util.h"

// fix the abominable naming of the context library functions using macros since
// wrapping them in functions causes problems (particularly for getcontext and
// makecontext, which seem to be required to be called with the same activation
// record or they will blow out the stack when control returns)
#define save_chkpt ::getcontext
#define update_chkpt ::makecontext
#define jump_to_chkpt ::setcontext

using namespace jmg;
using namespace std;
using namespace std::string_view_literals;

////////////////////////////////////////////////////////////////////////////////
// code for jumping to simple worker function
////////////////////////////////////////////////////////////////////////////////

using WorkerFcn = function<void(void)>;
using CtxtStack = array<uint8_t, 32768>;

using StackfulCtxt = tuple<ucontext_t, CtxtStack>;

void workerTrampoline(const intptr_t lambda_ptr_val) {
  // NOTE: Google told me to do this, it's not my fault
  auto* lambda_ptr = reinterpret_cast<std::function<void()>*>(lambda_ptr_val);
  JMG_ENFORCE(pred(lambda_ptr), "unable to trampoline to thread worker");
  (*lambda_ptr)();
}

void populateJumpTgt(StackfulCtxt& ctxt,
                     WorkerFcn& fcn,
                     ucontext_t& return_tgt) {
  auto& chkpt = std::get<0>(ctxt);
  memset(&chkpt, 0, sizeof(ucontext_t));
  auto& ctxt_stack = std::get<1>(ctxt);

  JMG_SYSTEM(save_chkpt(&chkpt), "unable to store return target"sv);

  chkpt.uc_link = &return_tgt;
  chkpt.uc_stack.ss_sp = ctxt_stack.data();
  chkpt.uc_stack.ss_size = ctxt_stack.size();

  update_chkpt(&chkpt, (void (*)())workerTrampoline, 1 /* argc */,
               reinterpret_cast<intptr_t>(&fcn));
}

TEST(UContextTests, TestWorkerContextJump) {
  atomic<bool> worker_executed = false;
  auto worker_fcn = WorkerFcn([&] mutable { worker_executed = true; });
  auto jump_tgt_ctxt = make_unique<StackfulCtxt>();

  ucontext_t return_chkpt;
  memset(&return_chkpt, 0, sizeof(return_chkpt));

  // NOTE: was_started flag is necessary to allow the code to determine which
  // context it is in after jump_to_chkpt has executed
  volatile bool was_started = false;

  JMG_SYSTEM(save_chkpt(&return_chkpt), "unable to store return checkpoint"sv);

  if (!was_started) {
    // main context has returned from the call to saveChkpt that stored the return
    // checkpoint and at this point it should jump to the worker function/lambda

    // indicate that the work has started so the subsequent return to this context
    // will be able to determine that it should check the condition and return
    was_started = true;

    populateJumpTgt(*jump_tgt_ctxt, worker_fcn, return_chkpt);

    auto& jump_tgt = std::get<ucontext_t>(*jump_tgt_ctxt);
    const auto rc = jump_to_chkpt(&jump_tgt);
    JMG_SYSTEM(rc, "unable to jump to target checkpoint");
    JMG_ENFORCE(-1 == rc, "setcontext returned a value [", rc,
                "] that was neither 0 nor -1");
    JMG_THROW_EXCEPTION(runtime_error, "unreachable");
  }

  // main context has resumed after the worker function has completed
  EXPECT_TRUE(worker_executed);
}

////////////////////////////////////////////////////////////////////////////////
// code for jumping to mock fiber function
////////////////////////////////////////////////////////////////////////////////

struct MockFbr {
  atomic<bool> executed = false;
};

using FbrFcn = function<void(MockFbr&)>;

void fiberTrampoline(const intptr_t lambda_ptr_val, const intptr_t fbr_ptr_val) {
  using namespace jmg;
  // NOTE: Google told me to do this, it's not my fault
  FbrFcn* lambda_ptr = reinterpret_cast<FbrFcn*>(lambda_ptr_val);
  MockFbr* fbr_ptr = reinterpret_cast<MockFbr*>(fbr_ptr_val);
  // take ownership of the lambda
  auto lambda_owner = unique_ptr<FbrFcn>(lambda_ptr);
  JMG_ENFORCE(pred(lambda_ptr),
              "unable to trampoline to mock fiber: bad lambda pointer");
  JMG_ENFORCE(pred(fbr_ptr),
              "unable to trampoline to mock fiber: bad fiber pointer");
  (*lambda_owner)(*fbr_ptr);
  // NOTE: lambda is destroyed on scope exit
}

using FbrCtxt = tuple<ucontext_t, CtxtStack, MockFbr>;

void populateJumpTgt(FbrCtxt& ctxt, FbrFcn&& fcn, ucontext_t& return_tgt) {
  auto& chkpt = std::get<ucontext_t>(ctxt);
  memset(&chkpt, 0, sizeof(ucontext_t));
  auto& ctxt_stack = std::get<CtxtStack>(ctxt);
  auto& fbr = std::get<MockFbr>(ctxt);

  JMG_SYSTEM(save_chkpt(&chkpt), "unable to store return target"sv);

  chkpt.uc_link = &return_tgt;
  chkpt.uc_stack.ss_sp = ctxt_stack.data();
  chkpt.uc_stack.ss_size = ctxt_stack.size();

  // take ownership of the fiber function
  auto fcn_owner = make_unique<FbrFcn>(std::move(fcn));

  update_chkpt(&chkpt, (void (*)())fiberTrampoline, 2 /* argc */,
               reinterpret_cast<intptr_t>(fcn_owner.get()),
               reinterpret_cast<intptr_t>(&fbr));
  // fiber function is now owned by the context and should be destroyed by the
  // jump target
  fcn_owner.release();
}

TEST(UContextTests, TestMockFiberContextJump) {
  auto fbr_fcn = FbrFcn([](MockFbr& fbr) mutable { fbr.executed = true; });
  auto jump_tgt_ctxt = make_unique<FbrCtxt>();

  ucontext_t return_chkpt;
  memset(&return_chkpt, 0, sizeof(return_chkpt));

  // NOTE: was_started flag is necessary to allow the code to determine which
  // context it is in after jump_to_chkpt has executed
  volatile bool was_started = false;

  JMG_SYSTEM(save_chkpt(&return_chkpt), "unable to store return checkpoint"sv);

  if (!was_started) {
    // main context has returned from the call to saveChkpt that stored the return
    // checkpoint and at this point it should jump to the worker function/lambda

    // indicate that the work has started so the subsequent return to this context
    // will be able to determine that it should check the condition and return
    was_started = true;

    populateJumpTgt(*jump_tgt_ctxt, std::move(fbr_fcn), return_chkpt);

    auto& jump_tgt = std::get<ucontext_t>(*jump_tgt_ctxt);
    const auto rc = jump_to_chkpt(&jump_tgt);
    JMG_SYSTEM(rc, "unable to jump to target checkpoint");
    JMG_ENFORCE(-1 == rc, "setcontext returned a value [", rc,
                "] that was neither 0 nor -1");
    JMG_THROW_EXCEPTION(runtime_error, "unreachable");
  }

  // main context has resumed after the worker function has completed
  auto& fbr = std::get<MockFbr>(*jump_tgt_ctxt);
  EXPECT_TRUE(fbr.executed);
}
