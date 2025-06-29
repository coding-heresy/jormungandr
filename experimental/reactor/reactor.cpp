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

#include "reactor.h"

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// headers specific to the initial epoll implementation
#include <fcntl.h>
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

using OptStrView = std::optional<std::string_view>;

namespace jmg
{

void Reactor::start() {
  // create the event pipe
  {
    int pipefds[2];
    memset(pipefds, 0, sizeof(pipefds));
    JMG_SYSTEM(pipe2(pipefds, O_NONBLOCK), "initializing pipe for posting ");
    read_fd_ = FileDescriptor(pipefds[0]);
    write_fd_ = FileDescriptor(pipefds[1]);
  }

  // set up the epoll fd
  // NOTE: store and forward idiom vs data member
  JMG_SYSTEM(
    [this] {
      const auto rslt = epoll_create1(0);
      if (rslt != 0) { epoll_fd_ = FileDescriptor(rslt); }
      return rslt;
    }(),
    "unable to create epoll file descriptor");

  // configure epoll to listen for events of interest
  EpollEvent event;
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  event.data.fd = unsafe(read_fd_);
  JMG_SYSTEM(epoll_ctl(unsafe(epoll_fd_), EPOLL_CTL_ADD, event.data.fd, &event),
             "unable to configure epoll to monitor the file descriptor for "
             "reading incoming control messages");

  bool was_started = false;

  // store the shutdown checkpoint
  storeCheckpoint(shutdown_checkpoint_, "store shutdown checkpoint");

  if (was_started) {
    // second return from getcontext, the reactor should be shut down at this point

    // TODO(bd) perform cleanup?

    // TODO(bd) sanity check the final state to ensure that all fibers are
    // inactive?

    // return control to the thread that started the reactor
    return;
  }

  // first return from getcontext, allocate the first fiber and jump to it

  // mark the reactor as having been started so the second return can exit
  was_started = true;

  const auto [id, block] = fiber_ctrl_.getOrAllocate();
  block.body.state = FiberState::kActive;
  active_fiber_id_ = id;

  auto initiator = FiberFcn([this] {
    // TODO(bd) any required initialization here before executing the scheduler
    schedule();
  });

  initNewFiber(block, initiator, "set up initial reactor fiber",
               &shutdown_checkpoint_);

  jumpTo(block, "initial reactor fiber");
}

void Reactor::shutdown() {
  static constexpr auto kShutdownCmd = static_cast<uint8_t>(Cmd::kShutdown);
  // store and forward idiom
  ssize_t write_sz = 0;
  JMG_SYSTEM(
    [&] {
      write_sz = write(unsafe(write_fd_), &kShutdownCmd, sizeof(kShutdownCmd));
      return write_sz;
    }(),
    "unable to send shutdown command to reactor");
  JMG_ENFORCE(sizeof(kShutdownCmd) == write_sz,
              "unexpected condition, there was no failure reported when "
              "sending the shutdown command to the reactor but ["
                << write_sz << "] bytes were written instead of the expected ["
                << sizeof(kShutdownCmd) << "]");
}

void Reactor::schedule(const std::optional<std::chrono::milliseconds> timeout) {
  static constexpr auto kNoTimeout = -1;

  EpollEvent event;
  memset(&event, 0, sizeof(event));

  // NOTE: store and forward idiom here
  int ready_fds_sz;
  JMG_SYSTEM(
    [&]() -> int {
      const auto timeout_duration =
        timeout ? static_cast<int>(timeout->count()) : kNoTimeout;
      ready_fds_sz = epoll_wait(unsafe(epoll_fd_), &event, 1 /* maxevents */,
                                timeout_duration);
      return ready_fds_sz;
    }(),
    "unable to wait for epoll events");

  JMG_ENFORCE(pred(ready_fds_sz) || pred(timeout),
              "unexpected condition: epoll_wait returned no events but there "
              "was no timeout originally set");
  JMG_ENFORCE(1 == ready_fds_sz,
              "unexpected condition, epoll_wait returned ["
                << ready_fds_sz
                << "] but should have returned [1] if there was no timeout");
  if (event.data.fd == unsafe(read_fd_)) {
    // TODO(bd) how many to read here?
    uint8_t buffer[256];
    // store and forward idiom
    ssize_t read_sz = 0;
    JMG_SYSTEM(
      [&] {
        read_sz = read(event.data.fd, &buffer, sizeof(buffer));
        // TODO(bd) explicitly handle EAGAIN/EWOULDBLOCK here?
        return read_sz;
      }(),
      "unable to read incoming command");
    if (0 == read_sz) {
      // read_fd_ was closed, should shut down
      is_shutdown_ = true;
      // TODO(bd) clean up?
      return;
    }

    for (int idx = 0; idx < read_sz; ++idx) {
      switch (static_cast<Cmd>(buffer[idx])) {
        case Cmd::kShutdown:
          is_shutdown_ = true;
          // TODO(bd) clean up?
          return;

        case Cmd::kPost:
          // TODO(bd) create another fiber to execute the incoming function
          break;
      }
    }
  }
  JMG_THROW_EXCEPTION(runtime_error,
                      "unexpected event on unknown file descriptor ["
                        << event.data.fd << "]");
}

void Reactor::trampoline(const intptr_t lambda_ptr_val) {
  // NOTE: Google told me to do this, it's not my fault
  auto* lambda_ptr = reinterpret_cast<std::function<void()>*>(lambda_ptr_val);
  JMG_ENFORCE(pred(lambda_ptr), "unable to trampoline to lambda");
  (*lambda_ptr)();
}

void Reactor::storeCheckpoint(ucontext_t& checkpoint,
                              const OptStrView operation) {
  auto msg = "unable to "s;
  str_append(msg, operation ? *operation : "store checkpoint"sv);
  JMG_SYSTEM(::getcontext(&checkpoint), msg);
}

void Reactor::jumpTo(const ucontext_t& checkpoint, const OptStrView tgt) {
  auto msg = "unable to jump to "s;
  str_append(msg, tgt ? *tgt : "target checkpoint"sv);
  const auto rslt = ::setcontext(&checkpoint);
  JMG_ENFORCE(rslt == -1,
              str_cat(msg, ", setcontext returned with a value other than -1"));
  JMG_THROW_SYSTEM_ERROR(msg);
}

void Reactor::initNewFiber(FiberCtrlBlock& block,
                           FiberFcn& fcn,
                           const OptStrView operation,
                           ucontext_t* returnTgt) {
  // for whatever reason, you need to store the current checkpoint using
  // getcontext and then modify it to point to the fiber function
  storeCheckpoint(block, operation);

  // point the updated context at the resources controlled by the provided fiber
  // control block
  auto& chkpt_stack = block.body.checkpoint.uc_stack;
  // TODO(bd) use variable size segmented stacks
  chkpt_stack.ss_sp = block.body.stack;
  chkpt_stack.ss_size = sizeof(block.body.stack);
  block.body.checkpoint.uc_link = returnTgt;

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
  ::makecontext(&(block.body.checkpoint), (void (*)())trampoline, 1 /* argc */,
                reinterpret_cast<intptr_t>(&fcn));
}

} // namespace jmg
