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

#include <memory>
#include <optional>

#include "jmg/conversion.h"
#include "jmg/preprocessor.h"
#include "uring.h"

using namespace std;

namespace jmg::uring
{

Event::Event(io_uring* ring, io_uring_cqe* cqe) : ring_(ring), cqe_(cqe) {
  if (cqe) {
    JMG_ENFORCE_USING(
      std::logic_error, pred(ring),
      "received a non-null CQE pointer with a null ring pointer");
  }
}

Event::~Event() {
  if (cqe_ && !is_cleanup_canceled_) { io_uring_cqe_seen(ring_, cqe_); }
}

Uring::Uring(const UringSz sz, const UringFlags flags) {
  memset(&ring_, 0, sizeof(ring_));
  JMG_SYSTEM(io_uring_queue_init(unsafe(sz), &ring_, unsafe(flags)),
             "unable to initialize io_uring queue");
}

Uring::~Uring() { io_uring_queue_exit(&ring_); }

Event Uring::awaitEvent(optional<Duration> timeout) {
  unique_ptr<UringDuration> wait_timeout;
  string is_timeout_str;
  {
    if (timeout) {
      UringDuration duration = from(*timeout);
      wait_timeout = make_unique<UringDuration>(duration);
      is_timeout_str = " no";
    }
  }
  // store and forward
  io_uring_cqe* cqe = nullptr;
  int rc = 0;
  JMG_SYSTEM_ERRNO_RETURN(
    [&]() -> int {
      rc = io_uring_wait_cqe_timeout(&ring_, &cqe, wait_timeout.get());
      return rc;
    }(),
    "unable to wait for io_uring completion with", is_timeout_str, " timeout");
  if (-ETIME == rc) {
    // TODO(bd) confirm that cqe is still nullptr?
    return Event();
  }
  JMG_ENFORCE(
    pred(cqe),
    "successfully waited for uring event but no event details were returned");
  return Event(&ring_, cqe);
}

void Uring::submitTimeoutReq(UserData data,
                             Duration timeout,
                             DelaySubmission isDelayed) {
  auto& sqe = getNextSqe();
  io_uring_sqe_set_data64(&sqe, unwrap(data));
  UringDuration timeout_duration = from(timeout);
  io_uring_prep_timeout(&sqe, &timeout_duration, 0 /* count */,
                        IORING_TIMEOUT_ETIME_SUCCESS);
  if (!unwrap(isDelayed)) { submitReq("timeout"sv); }
}

io_uring_sqe& Uring::getNextSqe() {
  auto* sqe = io_uring_get_sqe(&ring_);
  // TODO(bd) allow SQEs to be stored on a pending queue to be submitted when
  // the next ring slot becomes available
  JMG_ENFORCE(pred(sqe), "no submit queue entries currently available");
  return *sqe;
}

} // namespace jmg::uring
