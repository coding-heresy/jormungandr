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
#include "reactor.h"
#include "util.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

using OptStrView = std::optional<std::string_view>;

namespace jmg
{

void Reactor::start() {
  // create the eventfd
  notifier_ = [&] {
    // store and forward
    int fd;
    JMG_SYSTEM((fd = eventfd(0 /* initval */, EFD_NONBLOCK)),
               "unable to create eventfd");
    return EventFd(fd);
  }();

  // create the uring instance
  // TODO(bd) make uring size a parameter
  uring_ = make_unique<uring::Uring>(uring::UringSz(256));

  uring_->registerEventNotifier(*notifier_);

  auto initiator = FiberFcn([this] {
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

    auto [id, fcb] = fiber_ctrl_.getOrAllocate();
    fcb.body.state = FiberState::kActive;
    active_fiber_id_ = id;

    initFbr(fcb, initiator, "set up initial reactor fiber", &shutdown_chkpt_);
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
  JMG_ENFORCE_USING(logic_error, notifier_, "no notifier eventfd is available");
  detail::write_all(*notifier_, buffer_from(kShutdownCmd), "notifier eventfd"sv);
}

void Reactor::schedule(const std::optional<std::chrono::milliseconds> timeout) {
  JMG_ENFORCE_USING(logic_error, uring_,
                    "attempted to schedule fibers before starting the reactor");
  const auto event = [&] {
    std::optional<Duration> uring_timeout = nullopt;
    if (timeout) { *uring_timeout = from(*timeout); }
    return uring_->awaitEvent(uring_timeout);
  }();

  const auto& event_data = event.getUserData();

  if (notifier_
      && (static_cast<int>(unsafe(event_data)) == unsafe(*notifier_))) {
    // external thread has sent a message on the notifier event fd
    const auto data = [&] {
      uint64_t data;
      detail::read_all(*notifier_, buffer_from(data), "notifier eventfd"sv);
      return data;
    }();
    switch (data) {
      case unwrap(Cmd::kShutdown):
        // TODO(bd) execute shutdown sequence
        return;
      case unwrap(Cmd::kPost):
        JMG_THROW_EXCEPTION(
          runtime_error, "'post' message not yet implemented, stay tuned...");
      default:
        JMG_THROW_EXCEPTION(runtime_error, "unknown message type [", data, "]");
    }
  }
}

void Reactor::trampoline(const intptr_t lambda_ptr_val) {
  // NOTE: Google told me to do this, it's not my fault
  auto* lambda_ptr = reinterpret_cast<std::function<void()>*>(lambda_ptr_val);
  JMG_ENFORCE(pred(lambda_ptr), "unable to trampoline to lambda");
  (*lambda_ptr)();
}

void Reactor::saveChkpt(ucontext_t& chkpt, const OptStrView operation) {
  JMG_SYSTEM(::getcontext(&chkpt), "unable to ",
             operation ? *operation : "store checkpoint"sv);
}

void Reactor::jumpTo(FiberCtrlBlock& fcb, const OptStrView tgt) {
  auto msg = "unable to jump to "s;
  str_append(msg, (tgt ? *tgt : "target checkpoint"sv));
  // store and forward
  int rslt;
  JMG_SYSTEM(
    [&] {
      rslt = ::setcontext(&(fcb.body.chkpt));
      return rslt;
    }(),
    "unable to jump to ", (tgt ? *tgt : "target checkpoint"sv));
  JMG_ENFORCE(rslt == -1, "unable to jump to ",
              (tgt ? *tgt : "target checkpoint"sv),
              ", setcontext returned with a value other than -1");
  JMG_THROW_SYSTEM_ERROR("unreachable");
}

void Reactor::initFbr(FiberCtrlBlock& fcb,
                      FiberFcn& fcn,
                      const OptStrView operation,
                      ucontext_t* return_tgt) {
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
  // For reference, makecontext is a variadic
  // function that takes a ucontext_t*, a pointer to a function taking and
  // returning void, an argument count and a list of arguments whose length must
  // match the argument count. The C-style cast of the trampoline function is
  // required by the interface so there's no way around the sketchiness, but
  // that doesn't mean that I'm happy about it
  ::makecontext(&(fcb.body.chkpt), (void (*)())trampoline, 1 /* argc */,
                reinterpret_cast<intptr_t>(&fcn));
}

} // namespace jmg
