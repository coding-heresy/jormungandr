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

#define ENABLE_REACTOR_DEBUGGING_OUTPUT

#include <sys/eventfd.h>

#include <cstdint>

#include <tuple>

#include "jmg/conversion.h"
#include "jmg/meta.h"

#include "reactor.h"
#include "util.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace vws = std::ranges::views;

using OptStrView = std::optional<std::string_view>;

// fix the abominable naming of the context library functions using macros since
// wrapping them in functions causes problems (particularly for getcontext and
// makecontext, which seem to be required to be called with the same activation
// record or they will blow out the stack when control returns)
#define save_chkpt ::getcontext
#define update_chkpt ::makecontext
#define jump_to_chkpt ::setcontext

namespace
{

template<typename... Args>
void dbgOut(Args&&... args) {
#if defined(ENABLE_REACTOR_DEBUGGING_OUTPUT)
  cout << ">>>>> DBG ";
  (cout << ... << args);
  cout << endl;
#else
  ignore = make_tuple(std::forward<Args>(args)...);
#endif
}

template<typename T>
concept ReactorFcnT = jmg::SameAsDecayedT<jmg::Reactor::WorkerFcn, T>
                      || jmg::SameAsDecayedT<jmg::FiberFcn, T>;

/**
 * trampoline to a simple worker function that is called with no arguments and
 * returns no value
 */
void workerTrampoline(const intptr_t lambda_ptr_val) {
  try {
    // NOTE: Google told me to do this, it's not my fault
    auto* lambda_ptr = reinterpret_cast<std::function<void()>*>(lambda_ptr_val);
    JMG_ENFORCE(jmg::pred(lambda_ptr), "unable to trampoline to simple worker");
    (*lambda_ptr)();
  }
  // TODO(bd) seems likely that any exception in this function is inescapably
  // fatal but maybe there's a better way to handle it than this...
  catch (const exception& e) {
    std::cout << "ERROR: caught exception when jumping to worker function: "
              << e.what() << std::endl;
    throw;
  }
  catch (...) {
    std::cout << "ERROR: caught unexpected exception type ["
              << jmg::current_exception_type_name()
              << "] when jumping to worker function" << std::endl;
    throw;
  }
}

} // namespace

namespace jmg
{

#if defined(TMP_USE_NEW_FIBER_TRAMP)
namespace detail
{

/**
 * trampoline to a fiber function that is called with a reference to a fiber and
 * returns no value via the route of looking up the fiber control block and
 * executing the fiber function stored there
 */
void fiberTrampoline(const intptr_t reactor_ptr_val, const intptr_t fbr_id_val) {
  try {
    dbgOut("trampolining to fiber function via fiber [", fbr_id_val,
           "] associated with reactor at address [", reactor_ptr_val, "]");

    using namespace jmg;
    Reactor& reactor = *(reinterpret_cast<Reactor*>(reactor_ptr_val));
    const auto fbr_id = [&] -> FiberId {
      const auto intermediate = reinterpret_cast<int64_t>(fbr_id_val);
      return FiberId(static_cast<UnsafeTypeFromT<FiberId>>(intermediate));
    }();

    auto& fcb = reactor.fiber_ctrl_.getBlock(fbr_id);

    auto& fbr_fcn = fcb.body.fbr_fcn;
    JMG_ENFORCE_USING(logic_error, fbr_fcn,
                      "no fiber function saved when jumping into fiber [",
                      fbr_id, "]");
    (*fbr_fcn)(fcb.body.fbr);

    // clear out the fiber function that was executed
    fcb.body.fbr_fcn.reset();
  }
  // TODO(bd) seems likely that any exception in this function is inescapably
  // fatal but maybe there's a better way to handle it than this...
  catch (const exception& e) {
    std::cout << "ERROR: caught exception when jumping to fiber function: "
              << e.what() << std::endl;
    throw;
  }
  catch (...) {
    std::cout << "ERROR: caught unexpected exception type ["
              << current_exception_type_name()
              << "] when jumping to fiber function" << std::endl;
    throw;
  }
}

} // namespace detail
#else
/**
 * trampoline to a "fiber" function that is called with a reference to a fiber
 * and returns no value
 */
void fiberTrampoline(const intptr_t lambda_ptr_val, const intptr_t fbr_ptr_val) {
  dbgOut("trampolining to fiber function lambda at address [", lambda_ptr_val,
         "] with fiber argument at address [", fbr_ptr_val, "]");

  using namespace jmg;
  // NOTE: Google told me to do this, it's not my fault
  FiberFcn* lambda_ptr = reinterpret_cast<FiberFcn*>(lambda_ptr_val);
  Fiber* fbr_ptr = reinterpret_cast<Fiber*>(fbr_ptr_val);
  // take ownership of the lambda
  auto lambda_owner = unique_ptr<FiberFcn>(lambda_ptr);
  JMG_ENFORCE(pred(lambda_ptr),
              "unable to trampoline to fiber: bad lambda pointer");
  JMG_ENFORCE(pred(fbr_ptr),
              "unable to trampoline to fiber: bad fiber pointer");
  (*lambda_owner)(*fbr_ptr);
  // NOTE: lambda is destroyed on scope exit
}
#endif

Reactor::Reactor()
  // TODO(bd) uring size should be at settable at compile or run time
  : notifier_([&] {
    // store and forward
    int fd;
    JMG_SYSTEM((fd = eventfd(0 /* initval */, EFD_NONBLOCK)),
               "unable to create eventfd");
    return EventFd(fd);
  }())
  , runnable_(fiber_ctrl_) {}

void Reactor::start() {
  auto initiator = WorkerFcn([this] mutable {
    // NOTE: uring must be created inside the worker function because only one
    // thread can submit requests to it

    // TODO(bd) uring size should be at settable at compile or run time
    uring_ = make_unique<uring::Uring>(uring::UringSz(256));

    uring_->registerEventNotifier(notifier_);

    // TODO(bd) any required initialization here before executing the scheduler

    dbgOut("initial reactor fiber starting the scheduler");

    schedule();

    dbgOut("the scheduler has terminated while running fiber [",
           getActiveFbrId(), "]");
  });

  volatile bool was_started = false;

  // store the shutdown checkpoint
  JMG_SYSTEM(save_chkpt(&shutdown_chkpt_),
             "unable to store shutdown checkpoint");

  if (!was_started) {
    dbgOut("reactor is starting");

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

  dbgOut("reactor has shut down");

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
  // auto* lambda_ptr = new FiberFcn(std::move(fcn));
  auto lambda_ptr = make_unique<FiberFcn>(std::move(fcn));

  dbgOut("posting fiber function at address [",
         reinterpret_cast<uint64_t>(lambda_ptr.get()),
         "] to reactor for execution in a fiber");
  dbgOut("function address as octets [",
         str_join(buffer_from(reinterpret_cast<intptr_t>(lambda_ptr.get()))
                    | vws::transform(octetify),
                  " "sv, kOctetFmt),
         "]");

  // write the address of the lambda to the notifier eventfd to inform the
  // reactor of the work request
  detail::write_all(notifier_, buffer_from(lambda_ptr.get()),
                    "notifier eventfd");
  lambda_ptr.release();
}

void Reactor::schedule(const std::optional<std::chrono::milliseconds> timeout) {
  bool is_shutdown = false;
  // NOTE: the polling behavior is to handle the case where a fiber yields but
  // there are currently no other runnable fibers and no uring events have occurred
  auto is_polling = true;
  const auto active_fbr_id = getActiveFbrId();

  dbgOut("fiber [", active_fbr_id, "] is entering the scheduler");

  auto& active_fbr_fcb = fiber_ctrl_.getBlock(active_fbr_id);

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

        dbgOut("yielding fiber [", active_fbr_id, "]");

        active_fbr_fcb.body.state = FiberState::kRunnable;
        runnable_.enqueue(active_fbr_fcb);
        active_fbr_fcb.body.is_fiber_yielding = false;
      }
      if (next_active_id == active_fbr_id) {
        // active fiber is actually resuming without being blocked
        JMG_ENFORCE_USING(logic_error, !is_active_fbr_terminating,
                          "attempted to resume a terminated fiber");

        dbgOut("resuming active fiber [", active_fbr_id,
               "] instead of blocking");

        active_fbr_fcb.body.state = FiberState::kActive;
        return;
      }
      if (!is_active_fbr_terminating) {
        // active fiber is being blocked

        dbgOut("blocking fiber [", active_fbr_id, "]");

        active_fbr_fcb.body.state = FiberState::kBlocked;
        volatile auto pre_chkpt_id = unsafe(active_fbr_id);
        JMG_SYSTEM(save_chkpt(&(active_fbr_fcb.body.chkpt)),
                   "unable to store checkpoint when blocking active fiber");
        // NOTE: execution of a previously blocked fiber will return at this point
        if (pre_chkpt_id != unsafe(getActiveFbrId())) {
          ////////////////////
          // this is the case where a blocked fiber is being resumed
          ////////////////////
          const auto resumed_id =
            FiberId(static_cast<UnsafeTypeFromT<FiberId>>(pre_chkpt_id));
          auto& resumed_fcb = fiber_ctrl_.getBlock(resumed_id);
          resumed_fcb.body.state = FiberState::kActive;
          setActiveFbrId(resumed_fcb.id);

          dbgOut("resuming previously blocked fiber [", resumed_id, "]");

          return;
        }
      }
      else {
        // active fiber is being terminated, release its resources for reuse
        fiber_ctrl_.release(active_fbr_id);

        dbgOut("terminating fiber [", active_fbr_id, "]");
      }
      ////////////////////
      // execution should jump to the next active fiber
      ////////////////////

      dbgOut("resuming fiber [", next_active_id, "]");

      setActiveFbrId(next_active_id);
      jumpTo(next_active_fcb, "resuming fiber"sv);
      // jumpTo should NEVER return
      JMG_THROW_EXCEPTION(logic_error, "unreachable");
    }
    else {
      dbgOut("fiber [", active_fbr_id, "] is waiting for uring events");

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
        dbgOut("notification event detected by fiber [", active_fbr_id, "]");

        ////////////////////
        // external thread has sent a message on the notifier event fd
        ////////////////////
        const auto data = [&] {
          uint64_t data;
          detail::read_all(notifier_, buffer_from(data), "notifier eventfd"sv);
          dbgOut("incoming data as octets [",
                 str_join(buffer_from(data) | vws::transform(octetify), " "sv,
                          kOctetFmt),
                 "]");
          return data;
        }();
        if (unwrap(Cmd::kShutdown) == data) {
          ////////////////////
          // shutdown was requested
          ////////////////////

          dbgOut("shutdown requested");

          is_shutdown = true;
        }
        else {
          dbgOut("external work request received");

          ////////////////////
          // work requested, data is a pointer to an instance of FiberFcn that
          // the reactor must take control of and execute
          ////////////////////

          dbgOut("creating shared pointer for lambda stored at address [", data,
                 "]");

          // TODO(bd) shared pointer is never a great solution, maybe use a
          // moveable function object
          // auto fcn = shared_ptr<FiberFcn>(reinterpret_cast<FiberFcn*>(data));
          auto fcn = shared_ptr<FiberFcn>(reinterpret_cast<FiberFcn*>(data));

          dbgOut("creating internal wrapper for lambda stored at address [",
                 reinterpret_cast<intptr_t>(fcn.get()), "]");

          // create a wrapper FiberFcn that will include cleanup
          // TODO: OLD CODE auto wrapper = [this, fcn = std::move(fcn)](Fiber&
          // fbr) mutable {
          FiberFcn wrapper = [this, fcn = std::move(fcn)](Fiber& fbr) mutable {
            const auto fbr_id = fbr.getId();

            dbgOut("executing external work request for fiber [", fbr_id,
                   "] using lambda stored at address [",
                   reinterpret_cast<intptr_t>(fcn.get()), "]");

            // execute the wrapped handler
            (*fcn)(fbr);

            dbgOut("done executing external work request for fiber [", fbr_id,
                   "]");

            auto& fcb = fiber_ctrl_.getBlock(fbr_id);

            // terminate the current fiber
            fcb.body.state = FiberState::kTerminated;
            schedule();
          };

          // initialize a new fiber object
          auto& fcb =
            initFbr(std::move(wrapper), "execute externally requested work",
                    &shutdown_chkpt_);
          fcb.body.state = FiberState::kRunnable;

          dbgOut("new fiber [", fcb.id, "] was created");

          // enqueue the new fiber control block on the runnable queue
          runnable_.enqueue(fcb);
        }
      }
      else {
        dbgOut("uring event detected by fiber [", active_fbr_id, "]");

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

        dbgOut("uring event detected by fiber [", active_fbr_id,
               "] targeting fiber [", fbr_id, "]");

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

  dbgOut("fiber [", active_fbr_id, "] is exiting the scheduler");
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

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  JMG_SYSTEM(save_chkpt(&fcb.body.chkpt), "unable to save checkpoint",
             operation ? str_cat(" for ", *operation) : "");

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

#if defined(TMP_USE_NEW_FIBER_TRAMP)
FiberCtrlBlock& Reactor::initFbr(FiberFcn&& fcn,
                                 const OptStrView operation,
                                 ucontext_t* return_tgt) {
  // set up fiber control block
  auto [id, fcb] = fiber_ctrl_.getOrAllocate();

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  JMG_SYSTEM(save_chkpt(&fcb.body.chkpt), "unable to save checkpoint",
             operation ? str_cat(" for ", *operation) : "");

  // point the updated context at the resources controlled by the provided fiber
  // control block
  fcb.body.chkpt.uc_link = return_tgt;
  fcb.body.chkpt.uc_stack.ss_sp = fcb.body.stack.data();
  fcb.body.chkpt.uc_stack.ss_size = fcb.body.stack.size();

  // store the lambda that will be executed in the fiber body
  fcb.body.fbr_fcn = make_unique<FiberFcn>(fcn);

  void* fcb_fbr_ptr = static_cast<void*>(&fcb.body.fbr);
  // TODO(bd) avoid initializing the fiber more than once

  // initialize the Fiber using placement new
  [[maybe_unused]] auto* fbr = new (fcb_fbr_ptr) Fiber(id, *this);

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

  dbgOut("creating checkpoint with jump target trampoline to fiber [", id, "]");

  update_chkpt(&(fcb.body.chkpt), (void (*)())detail::fiberTrampoline,
               2 /* argc */, reinterpret_cast<intptr_t>(this),
               reinterpret_cast<intptr_t>(static_cast<int64_t>(unsafe(id))));
  return fcb;
}
#else
FiberCtrlBlock& Reactor::initFbr(FiberFcn&& fcn,
                                 const OptStrView operation,
                                 ucontext_t* return_tgt) {
  // set up fiber control block
  auto [id, fcb] = fiber_ctrl_.getOrAllocate();

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  JMG_SYSTEM(save_chkpt(&fcb.body.chkpt), "unable to save checkpoint",
             operation ? str_cat(" for ", *operation) : "");

  // point the updated context at the resources controlled by the provided fiber
  // control block
  fcb.body.chkpt.uc_link = return_tgt;
  fcb.body.chkpt.uc_stack.ss_sp = fcb.body.stack.data();
  fcb.body.chkpt.uc_stack.ss_size = fcb.body.stack.size();

  void* fcb_fbr_ptr = static_cast<void*>(&fcb.body.fbr);
  // TODO(bd) avoid initializing the fiber more than once

  // initialize the Fiber using placement new
  auto* fbr = new (fcb_fbr_ptr) Fiber(id, *this);

  // take ownership of the fiber function
  auto fcn_owner = make_unique<FiberFcn>(std::move(fcn));

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

  dbgOut(
    "creating checkpoint with jump target trampoline to lambda at address [",
    reinterpret_cast<intptr_t>(fcn_owner.get()),
    "] with argument fiber object at address [",
    reinterpret_cast<intptr_t>(fbr), "]");

  update_chkpt(&(fcb.body.chkpt), (void (*)())fiberTrampoline, 2 /* argc */,
               reinterpret_cast<intptr_t>(fcn_owner.get()),
               reinterpret_cast<intptr_t>(fbr));

  // fiber function is now owned by the context and will be destroyed by the
  // jump target
  fcn_owner.release();

  return fcb;
}
#endif

void Reactor::yieldFbr() {
  dbgOut("fiber [", getActiveFbrId(), "] is requesting to yield");

  auto& fcb = fiber_ctrl_.getBlock(getActiveFbrId());
  fcb.body.is_fiber_yielding = true;
  schedule();
}

} // namespace jmg
