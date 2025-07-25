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

#include <sys/eventfd.h>

#include <cstdint>

#include "jmg/conversion.h"
#include "jmg/meta.h"

#include "reactor.h"
#include "util.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

using OptStrView = std::optional<std::string_view>;

// fix the abominable naming of the context library functions using macros since
// wrapping them in functions causes problems (particularly for getcontext and
// makecontext, which seem to be required to be called with the same activation
// record or they will blow out the stack when control returns)
#define save_chkpt ::getcontext
#define update_chkpt ::makecontext
#define jump_to_chkpt ::setcontext

template<typename... Ts>
void dbgOut(Ts&&... args) {
  cout << "=====>>>>> ";
  cout << str_cat(std::forward<Ts>(args)...);
  cout << endl;
}

namespace
{
using PtrParts = std::pair<int32_t, int32_t>;

/**
 * split 64 bit pointer up into 32 bit low and high chunks for use when calling
 * update_chkpt/makecontext
 */
template<typename T>
PtrParts split_ptr(T& ref) {
  PtrParts rslt;
  int64_t addr = reinterpret_cast<int64_t>(&ref);
  rslt.first = static_cast<int32_t>(addr >> 32);
  rslt.second = static_cast<int32_t>(addr & 0xFFFFFFFF);
  return rslt;
}

template<typename T>
concept ReactorFcnT = jmg::SameAsDecayedT<jmg::Reactor::WorkerFcn, T>
                      || jmg::SameAsDecayedT<jmg::FiberFcn, T>;

// TODO(bd) understand why it appears that intptr_t can be used here while the
// other trampoline requires 32 bit integers
void workerTrampoline(const intptr_t lambda_ptr_val) {
  // NOTE: Google told me to do this, it's not my fault
  auto* lambda_ptr = reinterpret_cast<std::function<void()>*>(lambda_ptr_val);
  JMG_ENFORCE(jmg::pred(lambda_ptr), "unable to trampoline to thread worker");
  (*lambda_ptr)();
}

void fiberTrampoline(const int32_t lambda_ptr_high,
                     const int32_t lambda_ptr_low,
                     const int32_t fbr_ptr_high,
                     const int32_t fbr_ptr_low) {
  using namespace jmg;
  // NOTE: Google told me to do this, it's not my fault
  FiberFcn* lambda_ptr =
    reinterpret_cast<FiberFcn*>((static_cast<uint64_t>(lambda_ptr_high) << 32)
                                | lambda_ptr_low);
  Fiber* fbr_ptr =
    reinterpret_cast<Fiber*>((static_cast<uint64_t>(fbr_ptr_high) << 32)
                             | fbr_ptr_low);
  JMG_ENFORCE(pred(lambda_ptr),
              "unable to trampoline to fiber: bad lambda pointer");
  JMG_ENFORCE(pred(fbr_ptr),
              "unable to trampoline to fiber: bad fiber pointer");
  (*lambda_ptr)(*fbr_ptr);
}

} // namespace

namespace jmg
{

Reactor::Reactor()
  // TODO(bd) uring size should be at settable at compile or run time
  : notifier_([&] {
    // store and forward
    int fd;
    JMG_SYSTEM((fd = eventfd(0 /* initval */, EFD_NONBLOCK)),
               "unable to create eventfd");
    return EventFd(fd);
  }()) {}

void Reactor::start() {
  auto initiator = WorkerFcn([this] mutable {
    // NOTE: uring must be created inside the worker function because only one
    // thread can submit requests to it

    // TODO(bd) uring size should be at settable at compile or run time
    uring_ = make_unique<uring::Uring>(uring::UringSz(256));

    uring_->registerEventNotifier(notifier_);

    // TODO(bd) any required initialization here before executing the scheduler
    schedule();
  });

  volatile bool was_started = false;

  // store the shutdown checkpoint
  saveChkpt(shutdown_chkpt_, "store shutdown checkpoint");

  if (!was_started) {
    // first return from getcontext, allocate the first fiber and jump to it

    // mark the reactor as having been started so the second return can exit
    was_started = true;

    // auto [id, fcb] = fiber_ctrl_.getOrAllocate();
    // fcb.body.state = FiberState::kActive;
    // active_fiber_id_ = id;

    auto& fcb =
      initFbr(initiator, "set up initial reactor fiber", &shutdown_chkpt_);
    fcb.body.state = FiberState::kActive;
    setActiveFbrId(fcb.id);

    jumpTo(fcb, "initial reactor fiber");
  }
  // second return from getcontext, the reactor should be shut down at this point

  // TODO(bd) perform cleanup?

  // TODO(bd) sanity check the final state to ensure that all fibers are
  // inactive?

  // return control to the thread that started the reactor
}

void Reactor::shutdown() {
  static constexpr auto kShutdownCmd = unwrap(Cmd::kShutdown);
  detail::write_all(notifier_, buffer_from(kShutdownCmd), "notifier eventfd"sv);
}

void Reactor::post(FiberFcn&& fcn) {
  // TODO(bd) try to come up with a better way to do this
  auto* lambda_ptr = new FiberFcn(fcn);
  // write the address of the lambda to the notifier eventfd to inform the
  // reactor of the work request
  detail::write_all(notifier_, buffer_from(lambda_ptr), "notifier eventfd");
}

#if 1
void Reactor::schedule(const std::optional<std::chrono::milliseconds> timeout) {
  bool is_shutdown = false;
  // NOTE: the polling behavior is to handle the case where a fiber yields but
  // there are currently no other runnable fibers and no uring events have occurred
  auto is_polling = true;
  auto& active_fbr_fcb = fiber_ctrl_.getBlock(getActiveFbrId());

  dbgOut("scheduling from [", getActiveFbrId(), "]");

  JMG_ENFORCE_USING(
    logic_error,
    (FiberState::kActive == active_fbr_fcb.body.state)
      || (FiberState::kTerminated == active_fbr_fcb.body.state)
      || (FiberState::kBlocked == active_fbr_fcb.body.state),
    "scheduler invoked by a fiber that is not active, blocked or terminated");

  // TODO(bd) can it be assumed that a fiber whose state is kActive
  // when it calls schedule is yielding?

  const auto is_active_fbr_terminating =
    (active_fbr_fcb.body.state == FiberState::kTerminated);
  // TODO(bd) think carefully about the shutdown case, any outstanding events
  // should probably be allowed to drain
  while (!is_shutdown) {
    // TODO(bd) Do Something Smart(TM) to manage starvation risk
    if (!runnable_.empty()) {
      // resume execution of the first runnable fiber
      auto& next_active_fcb = runnable_.dequeue();
      const auto next_active_id = next_active_fcb.id;
      if (active_fbr_fcb.body.is_fiber_yielding) {
        JMG_ENFORCE_USING(logic_error, !is_active_fbr_terminating,
                          "attempted to yield a terminated fiber");
        JMG_ENFORCE_USING(logic_error,
                          FiberState::kActive == active_fbr_fcb.body.state,
                          "attempted to yield a non-active fiber");

        dbgOut("[", getActiveFbrId(), "] is yielding");

        active_fbr_fcb.body.state = FiberState::kRunnable;
        runnable_.enqueue(active_fbr_fcb);
        active_fbr_fcb.body.is_fiber_yielding = false;
      }
      if (next_active_id == getActiveFbrId()) {
        // active fiber is actually resuming without being blocked
        JMG_ENFORCE_USING(logic_error, !is_active_fbr_terminating,
                          "attempted to resume a terminated fiber");

        dbgOut("immediately resuming [", getActiveFbrId(), "]");

        active_fbr_fcb.body.state = FiberState::kActive;
        return;
      }
      if (!is_active_fbr_terminating) {
        // active fiber is being blocked

        dbgOut("[", getActiveFbrId(), "] is being blocked");

        active_fbr_fcb.body.state = FiberState::kBlocked;
        saveChkpt(active_fbr_fcb, "blocking active fiber"sv);
        // NOTE: execution of a previously blocked fiber will return at this point
        if (next_active_id != getActiveFbrId()) {
          ////////////////////
          // this is the case where a blocked fiber is being resumed
          ////////////////////
          next_active_fcb.body.state = FiberState::kActive;
          setActiveFbrId(next_active_id);
          return;
        }
      }
      else {
        // active fiber is being terminated, release its resources for reuse

        dbgOut("[", getActiveFbrId(), "] is terminating");

        fiber_ctrl_.release(getActiveFbrId());
      }
      ////////////////////
      // execution should jump to the next active fiber
      ////////////////////
      jumpTo(next_active_fcb, "resuming blocked fiber"sv);
      // jumpTo should NEVER return
      JMG_THROW_EXCEPTION(logic_error, "unreachable");
    }
    else {
      // TODO(bd) support awaiting a batch of events
      // poll or wait for uring events
      const auto event = [&] {
        std::optional<Duration> uring_timeout = nullopt;
        if (timeout && !is_polling) { *uring_timeout = from(*timeout); }
        return uring_->awaitEvent(uring_timeout);
      }();

      // polling should only occur on the first iteration of the loop
      is_polling = false;

      const auto& event_data = event.getUserData();

      if (static_cast<int>(unsafe(event_data)) == unsafe(notifier_)) {
        ////////////////////
        // external thread has sent a message on the notifier event fd
        ////////////////////
        const auto data = [&] {
          uint64_t data;
          detail::read_all(notifier_, buffer_from(data), "notifier eventfd"sv);
          return data;
        }();
        if (unwrap(Cmd::kShutdown) == data) {
          ////////////////////
          // shutdown was requested
          ////////////////////
          is_shutdown = true;
        }
        else {
          ////////////////////
          // work requested, data is a pointer to an instance of FiberFcn that
          // the reactor must take control of and execute
          ////////////////////

          // TODO(bd) shared pointer is never a great solution, maybe use a
          // moveable function object
          auto fcn = shared_ptr<FiberFcn>(reinterpret_cast<FiberFcn*>(data));

          // create a wrapper FiberFcn that will include cleanup
          auto wrapper = [this, fcn = std::move(fcn)](Fiber& fbr) mutable {
            const auto fbr_id = fbr.getId();

            // execute the wrapped handler
            (*fcn)(fbr);

            auto& fcb = fiber_ctrl_.getBlock(fbr_id);

            // terminate the current fiber
            fcb.body.state = FiberState::kTerminated;

            // TODO: OLD CODE fiber_ctrl_.release(fbr_id);

            schedule();
          };

          // initialize a new fiber object
          auto& fcb = initFbr(wrapper, "execute externally requested work");

          // enqueue the new fiber control block on the runnable queue
          runnable_.enqueue(fcb);
#if 0
          // jump to the wrapper FiberFcn
          jumpTo(fcb, "fiber for externally requested work");
#endif
        }
      }
      else {
        ////////////////////
        // uring event has occurred for a blocked fiber
        ////////////////////
        JMG_ENFORCE_USING(logic_error, unsafe(event_data) < fiber_ctrl_.count(),
                          "internal corruption, uring event appears to be "
                          "targeting a fiber with ID [",
                          unsafe(event_data),
                          "] but the largest available fiber ID is [",
                          fiber_ctrl_.count() - 1, "]");
        const auto fbr_id = FiberId(unsafe(event_data));
        auto& fcb = fiber_ctrl_.getBlock(fbr_id);
        JMG_ENFORCE_USING(logic_error, FiberState::kBlocked == fcb.body.state,
                          "received uring event for fiber [", fbr_id,
                          "] that was not blocked");
        fcb.body.state = FiberState::kRunnable;
        runnable_.enqueue(fcb);

        // TODO(bd) store the event in the FCB of the associated fiber
      }
    }
  }
}
#else
void Reactor::schedule(const std::optional<std::chrono::milliseconds> timeout) {
  bool is_shutdown = false;
  while (!is_shutdown) {
    const auto event = [&] {
      std::optional<Duration> uring_timeout = nullopt;
      if (timeout) { *uring_timeout = from(*timeout); }
      return uring_->awaitEvent(uring_timeout);
    }();

    const auto& event_data = event.getUserData();

    if (static_cast<int>(unsafe(event_data)) == unsafe(notifier_)) {
      // external thread has sent a message on the notifier event fd
      const auto data = [&] {
        uint64_t data;
        detail::read_all(notifier_, buffer_from(data), "notifier eventfd"sv);
        return data;
      }();
      if (unwrap(Cmd::kShutdown) == data) { is_shutdown = true; }
      else {
        // work requested, data is a pointer to an instance of FiberFcn that the
        // reactor must take control of and execute
        auto fcn = shared_ptr<FiberFcn>(reinterpret_cast<FiberFcn*>(data));

        // create a wrapper FiberFcn that will include cleanup
        auto wrapper = [this, fcn = std::move(fcn)](Fiber& fbr) mutable {
          const auto fbr_id = fbr.getId();

          // execute the wrapped handler
          (*fcn)(fbr);

          auto& fcb = fiber_ctrl_.getBlock(fbr_id);

          // terminate the current fiber
          fcb.body.state = FiberState::kTerminated;

          fiber_ctrl_.release(fbr_id);

          schedule();
        };

        // initialize a new fiber object
        auto& fcb = initFbr(wrapper, "execute posted work");

        // jump to the wrapper FiberFcn
        jumpTo(fcb, "posted work fiber");
      }
    }
  }
}
#endif

void Reactor::saveChkpt(ucontext_t& chkpt, const OptStrView operation) {
  JMG_SYSTEM(save_chkpt(&chkpt), "unable to ",
             operation ? *operation : "store checkpoint"sv);
}

void Reactor::jumpTo(FiberCtrlBlock& fcb, const OptStrView tgt) {
  auto msg = "unable to jump to "s;
  str_append(msg, (tgt ? *tgt : "target checkpoint"sv));
  // store and forward
  int rslt;
  JMG_SYSTEM(
    [&] {
      rslt = jump_to_chkpt(&(fcb.body.chkpt));
      return rslt;
    }(),
    "unable to jump to ", (tgt ? *tgt : "target checkpoint"sv));
  JMG_ENFORCE(rslt == -1, "unable to jump to ",
              (tgt ? *tgt : "target checkpoint"sv),
              ", jump_to (setcontext) returned with a value other than -1");
  JMG_THROW_SYSTEM_ERROR("unreachable");
}

FiberCtrlBlock& Reactor::initFbr(WorkerFcn& fcn,
                                 const OptStrView operation,
                                 ucontext_t* return_tgt) {
  // set up fiber control block
  auto [id, fcb] = fiber_ctrl_.getOrAllocate();
  fcb.body.state = FiberState::kActive;
  setActiveFbrId(id);

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  saveChkpt(fcb, operation);

  // point the updated context at the resources controlled by the provided fiber
  // control block
  fcb.body.chkpt.uc_link = return_tgt;
  fcb.body.chkpt.uc_stack.ss_sp = fcb.body.stack.data();
  fcb.body.chkpt.uc_stack.ss_size = fcb.body.stack.size();

  // update the previously stored checkpoint to cause it to jump to the provided
  // fiber function
  // NOTE: this is extremely sketchy.
  //
  // For reference, update_chkpt/makecontext is a variadic function
  // that takes a ucontext_t*, a pointer to a function taking and
  // returning void, an argument count and a list of arguments whose
  // length must match the argument count. The C-style cast of the
  // trampoline function is required by the interface so there's no
  // way around the sketchiness, but that doesn't mean that I'm happy
  // about it
  update_chkpt(&(fcb.body.chkpt), (void (*)())workerTrampoline, 1 /* argc */,
               reinterpret_cast<intptr_t>(&fcn));
  return fcb;
}

FiberCtrlBlock& Reactor::initFbr(FiberFcn&& fcn,
                                 const OptStrView operation,
                                 ucontext_t* return_tgt) {
  // set up fiber control block
  auto [id, fcb] = fiber_ctrl_.getOrAllocate();
  fcb.body.state = FiberState::kActive;
  setActiveFbrId(id);

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  saveChkpt(fcb, operation);

  // point the updated context at the resources controlled by the provided fiber
  // control block
  fcb.body.chkpt.uc_link = return_tgt;
  fcb.body.chkpt.uc_stack.ss_sp = fcb.body.stack.data();
  fcb.body.chkpt.uc_stack.ss_size = fcb.body.stack.size();

  void* fcb_fbr_ptr = static_cast<void*>(&fcb.body.fbr);
  // TODO(bd) avoid initializing the fiber more than once

  // initialize the Fiber using placement new
  auto* fbr = new (fcb_fbr_ptr) Fiber(id, *this);

  auto fcn_parts = split_ptr(fcn);
  auto fbr_parts = split_ptr(*fbr);

  // update the previously stored checkpoint to cause it to jump to the provided
  // fiber function
  // NOTE: this is extremely sketchy.
  //
  // For reference, update_chkpt/makecontext is a variadic function
  // that takes a ucontext_t*, a pointer to a function taking and
  // returning void, an argument count and a list of arguments whose
  // length must match the argument count. The C-style cast of the
  // trampoline function is required by the interface so there's no
  // way around the sketchiness, but that doesn't mean that I'm happy
  // about it
  update_chkpt(&(fcb.body.chkpt), (void (*)())fiberTrampoline, 4 /* argc */,
               fcn_parts.first, fcn_parts.second, fbr_parts.first,
               fbr_parts.second);
  return fcb;
}

void Reactor::yieldFbr() {
  auto& fcb = fiber_ctrl_.getBlock(getActiveFbrId());
  fcb.body.is_fiber_yielding = true;
  schedule();
}

// TODO(bd) remove
//
// void Reactor::dbgLog(const string_view str) {
//   static constexpr auto kDbgPrefix = "DBG "sv;
//   static const auto kVecPrototype = [&] {
//     array<struct iovec, 2> rslt;
//     rslt[0].iov_base = as_void_ptr(kDbgPrefix.data());
//     rslt[0].iov_len = kDbgPrefix.size();
//     return rslt;
//   }();
//   auto io_array = kVecPrototype;
//   io_array[1].iov_base = as_void_ptr(str.data());
//   io_array[1].iov_len = str.size();
//   uring_->submitWriteReq(kStdoutFd, span(io_array));
//   // TODO(bd) this is dangerous because some other event could occur before the
//   // write, either use writev for a fully blocking write or make this function
//   // non-blocking
//   const auto event = uring_->awaitEvent(from(100ms));
//   JMG_ENFORCE(pred(event), "timed out waiting for event");
//   if (event->res < 0) { JMG_THROW_SYSTEM_ERROR(-event->res); }
// }

} // namespace jmg
