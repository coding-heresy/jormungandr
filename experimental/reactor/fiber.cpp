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
#include "reactor.h"

#include <exception>

using namespace std;

namespace jmg
{

Fiber::Fiber(FiberId id, Reactor& reactor)
  : id_(id)
  , reactor_(&reactor)
  , fcb_body_(&(reactor.fiber_ctrl_.getBlock(id).body)) {}

void Fiber::yield() { reactor_->yieldFbr(); }

FileDescriptor Fiber::open_file(const std::filesystem::path& file_path,
                                const FileOpenFlags flags,
                                const std::optional<mode_t> permissions) {
  mode_t mode = permissions ? *permissions : static_cast<mode_t>(0);
  auto& uring = *(reactor_->uring_);
  // set the user data to the fiber ID so the completion event gets routed back
  // to this thread
  uring.submitFileOpenReq(c_string_view(file_path.native()), flags, mode,
                          uring::UserData(unsafe(id_)));
  // enter the scheduler to defer further processing until the operation is complete
  reactor_->schedule();
  JMG_ENFORCE_USING(logic_error, pred(fcb_body_->event),
                    "internal corruption, event returned by reactor for file "
                    "open request has no request completion info");
  auto cleanup = Cleanup([&]() {
    // TODO(bd) fix this, FiberCtrlBlockBody should probably hold the
    // uring::Event instance using unique_ptr
    fcb_body_->event = uring::Event();
  });
  const auto user_data = fcb_body_->event.getUserData();
  JMG_ENFORCE(unsafe(user_data) == unsafe(id_),
              "mismatch between current fiber ID [", id_, "] and fiber ID [",
              unsafe(user_data),
              "] associated with file open completion event");

  const auto& cqe = *(fcb_body_->event);

  // TODO(bd) investigate cqe.res to determine if there was an error attempting
  // to complete the request (the value of cqe.res is equivalent to -errno if it
  // is less than 0

  return FileDescriptor(cqe.res);
}

} // namespace jmg
