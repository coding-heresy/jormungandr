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
#define create_fbr_chkpt ::makecontext
#define jump_to_chkpt ::swapcontext

namespace
{

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
    auto* lambda_ptr = reinterpret_cast<function<void()>*>(lambda_ptr_val);
    JMG_ENFORCE(jmg::pred(lambda_ptr), "unable to trampoline to simple worker");
    (*lambda_ptr)();
  }
  // TODO(bd) seems likely that any exception in this function is inescapably
  // fatal but maybe there's a better way to handle it than this...
  JMG_SINK_ALL_EXCEPTIONS("worker function jump")
}

/**
 * helper function that simplifies jumping between checkpoints
 */
void chkptStoreAndJump(ucontext_t& src,
                       ucontext_t& tgt,
                       const string_view description,
                       const bool is_return_allowed = true) {
  // store and forward
  int rslt;
  JMG_SYSTEM(
    [&] {
      rslt = jump_to_chkpt(&src, &tgt);
      return rslt;
    }(),
    "unable to jump to ", description);
  if (is_return_allowed && (0 == rslt)) { return; }
  JMG_ENFORCE(rslt == -1, "unable to jump to ", description,
              ", jump_to_chkpt "
              "(swapcontext) returned with value [",
              rslt, "] instead of either 0 or -1");
  JMG_THROW_SYSTEM_ERROR("unable to jump to ", description);
}

} // namespace

namespace jmg
{

using namespace reactor;

namespace detail
{

/**
 * trampoline to a fiber function that is called with a reference to a fiber and
 * returns no value via the route of looking up the fiber control block and
 * executing the fiber function stored there
 */
void fiberTrampoline(const intptr_t reactor_ptr_val, const intptr_t fbr_id_val) {
  try {
    using namespace jmg;
    Reactor& reactor = *(reinterpret_cast<Reactor*>(reactor_ptr_val));
#if defined(JMG_REACTOR_DEBUG_TRAMPOLINES)
    JMG_URING_LOG_DEBUG(reactor.uring_,
                        "trampolining to fiber function via fiber [",
                        fbr_id_val, "] associated with reactor at address [",
                        reactor_ptr_val, "]");
#endif
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
  JMG_SINK_ALL_EXCEPTIONS("fiber function jump")
}

/**
 * correctly handle notification data of any size
 */
template<typename T>
void sendNotification(PipeWriteFd post_src, const T val) {
  if constexpr (sizeof(val) < sizeof(uint64_t)) {
    uint64_t send_val = unwrap(val);
    write_all(post_src, buffer_from(send_val), "notifier pipe"sv);
  }
  else {
    static_assert(
      sizeof(val) == sizeof(uint64_t),
      "notification value size must be less than or equal to 8 octets");
    write_all(post_src, buffer_from(val), "notifier pipe"sv);
  }
}

} // namespace detail

Reactor::Reactor(const size_t thread_pool_worker_count)
  // TODO(bd) uring size should be at settable at compile or run time
  : runnable_(fiber_ctrl_), thread_pool_(thread_pool_worker_count) {
  const auto [read_fd, write_fd] = make_pipe();
  post_tgt_ = read_fd;
  post_src_ = write_fd;
  notifier_vec_[0].iov_base = reinterpret_cast<void*>(&notifier_data_);
  notifier_vec_[0].iov_len = sizeof(notifier_data_);
}

Reactor::~Reactor() {
  // TODO(bd) maybe support a timeout for the thread pool join?
  thread_pool_.join();
}

void Reactor::start() {
  auto initiator = WorkerFcn([this] mutable {
    // NOTE: uring must be created inside the worker function because only one
    // thread can submit requests to it

    // TODO(bd) uring size should be at settable at compile or run time
    uring_ = make_unique<uring::Uring>(uring::UringSz(256));

    // issue the first iouring read request for the notifier
    resetNotifier();

    // TODO(bd) any required initialization here before executing the scheduler

    JMG_URING_LOG_DEBUG(uring_, "initial reactor fiber starting the scheduler");

    schedule();

    JMG_URING_LOG_DEBUG(uring_,
                        "the scheduler has terminated while running fiber [",
                        getActiveFbrId(), "]");
  });

  // store the shutdown checkpoint
  JMG_SYSTEM(save_chkpt(&shutdown_chkpt_),
             "unable to store shutdown checkpoint");

#if defined(JMG_ENABLE_REACTOR_DEBUGGING_OUTPUT)
  // NOTE: JMG_URING_LOG_DEBUG can't be used here because the io_uring
  // instance hasn't yet been initialized
  cout << ">>>>> DBG reactor is starting" << endl;
#endif

  auto& fcb =
    initFbr(initiator, "set up initial reactor fiber", &shutdown_chkpt_);
  // TODO(bd) maybe rethink initial state?
  // reactor starts with no initial work, mark the initial thread as 'blocked'
  fcb.body.state = FiberState::kBlocked;
  setActiveFbrId(fcb.id);

  chkptStoreAndJump(shutdown_chkpt_, fcb.body.chkpt, "initial reactor fiber");

  JMG_URING_LOG_DEBUG(uring_, "reactor has shut down");

  // second return from getcontext, the reactor should be shut down at this point

  // TODO(bd) perform cleanup?

  // TODO(bd) sanity check the final state to ensure that all fibers are
  // inactive?

  // return control to the thread that started the reactor
}

void Reactor::shutdown() { detail::sendNotification(post_src_, kShutdownCmd); }

void Reactor::execute(FiberFcn&& fcn) {
  // TODO(bd) try to come up with a better way to do this
  auto lambda_ptr = make_unique<FiberFcn>(std::move(fcn));

#if defined(JMG_REACTOR_DEBUG_TRAMPOLINES)
  JMG_URING_LOG_DEBUG(
    uring_, "posting fiber function at address [",
    reinterpret_cast<uint64_t>(lambda_ptr.get()),
    "] to reactor for execution in a fiber using file descriptor [", post_src_,
    "]");
  JMG_URING_LOG_DEBUG(
    uring_, "function address as octets [",
    str_join(buffer_from(reinterpret_cast<intptr_t>(lambda_ptr.get()))
               | vws::transform(octetify),
             " "sv, kOctetFmt),
    "]");
#endif

  // write the address of the lambda to the notifier eventfd to inform the
  // reactor of the work request
  detail::sendNotification(post_src_, lambda_ptr.get());
  lambda_ptr.release();
}

void Reactor::schedule() {
  const auto active_fbr_id = getActiveFbrId();
  auto& active_fbr_fcb = fiber_ctrl_.getBlock(active_fbr_id);
  auto active_fbr_state = active_fbr_fcb.body.state;

  switch (active_fbr_state) {
    case FiberState::kActive:
      JMG_URING_LOG_DEBUG(uring_, "scheduler invoked by active fiber [",
                          active_fbr_id, "]");
      active_fbr_fcb.body.state = FiberState::kBlocked;
      break;
    case FiberState::kYielding:
      JMG_URING_LOG_DEBUG(uring_, "scheduler invoked by yielding fiber [",
                          active_fbr_id, "]");
      // NOTE: do not enqueue this fiber until after uring event
      // processing
      break;
    case FiberState::kTerminated:
      JMG_URING_LOG_DEBUG(uring_, "scheduler invoked by terminated fiber [",
                          active_fbr_id, "]");
      break;
    case FiberState::kBlocked:
      JMG_URING_LOG_DEBUG(uring_, "scheduler invoked by blocked fiber [",
                          active_fbr_id, "]");
      break;
    default:
      JMG_THROW_EXCEPTION(logic_error,
                          "scheduler invoked by a fiber with invalid state [",
                          active_fbr_state, "]");
  }

  auto is_shutdown = false;
  while (!is_shutdown) {
    JMG_URING_LOG_DEBUG(uring_, "fiber [", active_fbr_id,
                        "] is starting new iteration of scheduler loop");
    // TODO(bd) tune the rates at which the ring and the run queue are
    // serviced

    // TODO(bd) for now, always try to pull in 1 event before
    // servicing the run queue, to avoid starving the ring
    {
      auto opt_event = [&] -> optional<uring::Event> {
        if (!runnable_.empty()) {
          // there are runnable fibers, quickly check for ring events
          if (!uring_->hasEvent()) { return nullopt; }
        }
        // either there are no runnable fibers, or there is an event
        // available
        return uring_->awaitEvent();
      }();

      if (opt_event) {
        auto& event = *opt_event;
        auto user_data = event.getUserData();
        FiberCtrlBlock* runnable_fcb = nullptr;

        if (isDetachedEventFailureReport(user_data)) {
          JMG_URING_LOG_DEBUG(uring_, "some detached operation failed");
          // TODO(bd) log some information here
        }
        else if (isNotification(user_data)) {
          if (isShutdownNotified()) {
            JMG_URING_LOG_DEBUG(uring_, "shutdown signal received");
            is_shutdown = true;
          }
          else {
            auto reseter = Cleanup([&]() { resetNotifier(); });
            if (const auto notified_fbr = tryGetNotifiedFiber(); notified_fbr) {
              ////////////////////
              // prior compute request completed
              const auto fbr_id = *notified_fbr;
              JMG_URING_LOG_DEBUG(
                uring_, "thread pool computation completed for fiber [", fbr_id,
                "]");
              auto& fcb = fiber_ctrl_.getBlock(fbr_id);
              JMG_ENFORCE_USING(logic_error,
                                FiberState::kBlocked == fcb.body.state,
                                "received incoming data for fiber [", fbr_id,
                                "] that was not blocked");
              fcb.body.state = FiberState::kRunnable;
              runnable_fcb = &fcb;
            }
            else {
              ////////////////////
              // work requested, data is a pointer to an instance of
              // FiberFcn that the reactor must take control of and
              // execute
              auto fcn = getWrappedFiberFcnFromNotifier();
              auto& fcb =
                initFbr(std::move(fcn), "executing externally requested work",
                        &shutdown_chkpt_);
              JMG_URING_LOG_DEBUG(uring_, "new task assigned to fiber [",
                                  fcb.id, "]");
              fcb.body.state = FiberState::kEmbryonic;
              runnable_fcb = &fcb;
            }
          }
        }
        else {
          // ring completion event
          const auto fbr_id = FiberId(unsafe(user_data));
          JMG_URING_LOG_DEBUG(uring_, "uring completion event for fiber [",
                              fbr_id, "]");
          auto& fcb = fiber_ctrl_.getBlock(fbr_id);
          fcb.body.state = FiberState::kRunnable;
          // save the event in the fiber control block so it can be
          // accessible when the fiber resumes
          fcb.body.event = std::move(*opt_event);
          runnable_fcb = &fcb;
        }
        if (runnable_fcb) {
          JMG_URING_LOG_DEBUG(uring_, "enqueued runnable fiber [",
                              runnable_fcb->id, "]");
          runnable_.enqueue(*runnable_fcb);
        }
      }
      // NOTE: Event object (if any) will be consumed at this point
    }

    // finished servicing the ring, now service the run queue
    if (!is_shutdown) {
      if (FiberState::kYielding == active_fbr_state) {
        active_fbr_fcb.body.state = FiberState::kRunnable;
        runnable_.enqueue(active_fbr_fcb);
        JMG_URING_LOG_DEBUG(uring_, "enqueued yielding fiber [", active_fbr_id,
                            "]");
      }
      JMG_ENFORCE_USING(logic_error, !runnable_.empty(),
                        "run queue was empty after io_uring event processing");
      auto& fcb = runnable_.dequeue();
      const auto fbr_id = fcb.id;
      JMG_URING_LOG_DEBUG(uring_, "dequeued runnable fiber [", fbr_id, "]");
      const auto is_embryonic = (FiberState::kEmbryonic == fcb.body.state);
      fcb.body.state = FiberState::kActive;
      if (fbr_id != active_fbr_id) {
        if (is_embryonic) {
          JMG_URING_LOG_DEBUG(uring_, "starting new fiber [", fbr_id, "]");
        }
        else {
          //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          // jump from current fiber to newly reactivated fiber
          JMG_URING_LOG_DEBUG(uring_, "resuming blocked fiber [", fbr_id, "]");
        }
        jumpTo(fcb, "resuming fiber"sv);
      }
      ////////////////////
      // resume state of active_fbr_id
      JMG_URING_LOG_DEBUG(uring_, "resuming active fiber [", active_fbr_id, "]");
      return;
    }
  }
}

void Reactor::jumpTo(FiberCtrlBlock& fcb, const OptStrView tgt) {
  auto msg = "unable to jump to "s;
  str_append(msg, (tgt ? *tgt : "target checkpoint"sv));

  const auto yielding_fbr_id = getActiveFbrId();
  auto& yielding_fbr_fcb = fiber_ctrl_.getBlock(yielding_fbr_id);
  const auto next_active_id = fcb.body.fbr.getId();
  fcb.body.state = FiberState::kActive;

  JMG_URING_LOG_DEBUG(uring_, "jumping from fiber [", yielding_fbr_id,
                      "] to fiber [", next_active_id, "]");

  setActiveFbrId(next_active_id);
  chkptStoreAndJump(yielding_fbr_fcb.body.chkpt, fcb.body.chkpt,
                    "new or resuming fiber");
  JMG_URING_LOG_DEBUG(uring_, "resuming previously yielded fiber [",
                      yielding_fbr_id, "]");
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
  // For reference, create_fbr_chkpt/makecontext is a variadic function
  // that takes a ucontext_t*, a pointer to a function taking and
  // returning void, an argument count and a list of arguments whose
  // length must match the argument count. The C-style cast of the
  // trampoline function is required by the interface so there's no
  // way around the sketchiness, but that doesn't mean that I'm happy
  // about it
  create_fbr_chkpt(&(fcb.body.chkpt), (void (*)())workerTrampoline,
                   1 /* argc */, reinterpret_cast<intptr_t>(&fcn));
  return fcb;
}

FiberCtrlBlock& Reactor::initFbr(FiberFcn&& fcn,
                                 const OptStrView operation,
                                 ucontext_t* return_tgt) {
  // set up fiber control block
  auto [id, fcb] = fiber_ctrl_.getOrAllocate();

  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  JMG_SYSTEM(save_chkpt(&fcb.body.chkpt), "unable to prepare checkpoint",
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
  // For reference, create_fbr_chkpt/makecontext is a variadic function
  // that takes a ucontext_t*, a pointer to a function taking and
  // returning void, an argument count and a list of arguments whose
  // length must match the argument count. The C-style cast of the
  // trampoline function is required by the interface so there's no
  // way around the sketchiness, but that doesn't mean that I'm happy
  // about it

  JMG_URING_LOG_DEBUG(
    uring_, "creating checkpoint with jump target trampoline to fiber [", id,
    "]");

  create_fbr_chkpt(&(fcb.body.chkpt), (void (*)())detail::fiberTrampoline,
                   2 /* argc */, reinterpret_cast<intptr_t>(this),
                   reinterpret_cast<intptr_t>(static_cast<int64_t>(unsafe(id))));
  return fcb;
}

void Reactor::yieldFbr() {
  JMG_URING_LOG_DEBUG(uring_, "fiber [", getActiveFbrId(),
                      "] is requesting to yield");

  auto& fcb = fiber_ctrl_.getBlock(getActiveFbrId());
  fcb.body.state = FiberState::kYielding;
  schedule();
}

void Reactor::resetNotifier() {
  // NOTE: use the negative value of the post_tgt_ file descriptor for the user
  // data to ensure that there is no collision with set of fiber IDs, which are
  // passed as user data to operations that need to be mapped back to a specific
  // fiber
  uring_->submitReadReq(post_tgt_, span(notifier_vec_),
                        uring::UserData(-unsafe(post_tgt_)));
}

void Reactor::notifyFbr(const FiberId id) {
  detail::sendNotification(post_src_, id);
}

bool Reactor::isDetachedEventFailureReport(const uring::UserData user_data) {
  return uring::kDetachedOperationFailure == user_data;
}

bool Reactor::isNotification(const uring::UserData user_data) {
  return static_cast<int>(unsafe(user_data)) == -unsafe(post_tgt_);
}

bool Reactor::isShutdownNotified() { return kShutdownCmd == notifier_data_; }

std::optional<FiberId> Reactor::tryGetNotifiedFiber() {
  if (notifier_data_ > kMaxFibers) { return nullopt; }
  return FiberId(notifier_data_);
}

FiberFcn Reactor::getWrappedFiberFcnFromNotifier() {
  // TODO(bd) shared pointer is never a great solution, maybe use a
  // moveable function object
  auto fcn = shared_ptr<FiberFcn>(reinterpret_cast<FiberFcn*>(notifier_data_));

  // create a wrapper FiberFcn that will include cleanup
  FiberFcn wrapper = [this, fcn = std::move(fcn)](Fiber& fbr) mutable {
    const auto fbr_id = fbr.getId();

    // execute the wrapped handler
    (*fcn)(fbr);

    // terminate the current fiber
    auto& fcb = fiber_ctrl_.getBlock(fbr_id);
    fcb.body.state = FiberState::kTerminated;
    fiber_ctrl_.release(fbr_id);
    schedule();
  };
  return wrapper;
}

} // namespace jmg
