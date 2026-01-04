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

#include "fiber.h"

#include <exception>

#include "reactor.h"

using namespace std;

namespace jmg
{

Fiber::Fiber(FiberId id, Reactor& reactor)
  : id_(id)
  , reactor_(&reactor)
  , fcb_body_(&(reactor.fiber_ctrl_.getBlock(id).body)) {}

void Fiber::yield() { reactor_->yieldFbr(); }

FileDescriptor Fiber::openFile(const std::filesystem::path& file_path,
                               const FileOpenFlags flags,
                               const std::optional<mode_t> permissions) {
  mode_t mode = permissions ? *permissions : static_cast<mode_t>(0);
  auto& uring = *(reactor_->uring_);

  // set the user data to the fiber ID so the completion event gets routed back
  // to this thread
  uring.submitFileOpenReq(c_string_view(file_path.native()), flags, mode,
                          uring::UserData(unsafe(id_)));

  ////////////////////
  // enter the scheduler to defer further processing until the operation is
  // complete
  reactor_->schedule();

  ////////////////////
  // return from scheduler
  const auto event = getEvent("open file"sv);

  const auto& cqe = *(event);
  if (cqe.res < 0) {
    JMG_THROW_SYSTEM_ERROR_FROM_ERRNO(-cqe.res, "failed to open file [",
                                      file_path.native(), "]");
  }

  return FileDescriptor(cqe.res);
}

void Fiber::close(FileDescriptor fd) {
  auto& uring = *(reactor_->uring_);

  // set the user data to the fiber ID so the completion event gets routed back
  // to this thread
  uring.submitFdCloseReq(fd, uring::UserData(unsafe(id_)));

  ////////////////////
  // enter the scheduler to defer further processing until the operation is
  // complete
  reactor_->schedule();

  const auto event = getEvent("close file descriptor");

  // NOTE: there is no return value and all checks have been performed by
  // getEvent, no need for further action and destructor for Event will take
  // care of cleanup
}

void Fiber::write(const FileDescriptor fd, const BufferView data) {
  using namespace uring;
  if (data.empty()) { return; }
  auto& uring = *(reactor_->uring_);

  struct iovec iov[1];
  iov[0].iov_base = (void*)data.data();
  iov[0].iov_len = data.size();

  // set the user data to the fiber ID so the completion event gets routed back
  // to this thread
  uring.submitWriteReq(fd, iov, DelaySubmission::kNoDelay,
                       UserData(unsafe(id_)));

  ////////////////////
  // enter the scheduler to defer further processing until the operation is
  // complete
  reactor_->schedule();

  const auto event = getEvent("write data");

  // NOTE: there is no return value and all checks have been performed by
  // getEvent, no need for further action and destructor for Event will take
  // care of cleanup
}

uring::Event Fiber::getEvent(const std::string_view op) {
  // check for missing event
  JMG_ENFORCE_USING(
    logic_error, pred(fcb_body_->event),
    "internal corruption, event returned by reactor for request to ", op,
    " has no request completion info");

  auto event = std::move(fcb_body_->event);
  // TODO(bd) fix this, FiberCtrlBlockBody should probably hold the
  // uring::Event instance using unique_ptr
  fcb_body_->event = uring::Event();

  {
    // verify that event user data matches the current fiber ID
    const auto user_data = event.getUserData();
    JMG_ENFORCE(unsafe(user_data) == unsafe(id_),
                "mismatch between current fiber ID [", id_, "] and fiber ID [",
                unsafe(user_data),
                "] associated with file open completion event");
  }

  {
    // check for failure of the operation in the kernel
    const auto& cqe = *event;
    if (cqe.res < 0) {
      JMG_THROW_SYSTEM_ERROR_FROM_ERRNO(-cqe.res, "failed to ", op);
    }
  }

  // cancel the cleanup operation, the destructor for the returned Event object
  // will handle the cleanup
  return event;
}

} // namespace jmg
