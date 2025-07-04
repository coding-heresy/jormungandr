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

#include <gmock/gmock.h>

#include "jmg/conversion.h"
#include "jmg/future.h"
#include "jmg/types.h"
#include "uring.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_literals;

TEST(UringTests, SmokeTest) {
  constexpr auto user_data = uring::UserData(42);
  uring::Uring uring(uring::UringSz(256));
  uring.submitTimeoutReq(user_data, from(10ms));
  const auto begin_ts = getCurrentTime();
  const auto event = uring.awaitEvent(from(100ms));
  const auto end_ts = getCurrentTime();
  EXPECT_TRUE(pred(event)) << "timed out waiting for event that should have "
                              "occurred before the timeout";
  EXPECT_EQ(unsafe(user_data), unsafe(event.getUserData()));

  const auto diff = end_ts - begin_ts;
  chrono::microseconds chrono_diff = from(diff);
  EXPECT_TRUE(chrono_diff.count() >= 10 * kMicroSecPerMillisec)
    << "event duration was less than expected timeout";
}

// TODO(bd) this test currently fails
#if 0
TEST(UringTests, MsgTest) {
  auto prm = make_unique<Promise<uring::UserData>>();
  auto ftr = prm->get_future();
  auto uring = make_unique<uring::Uring>(uring::UringSz(256));
  const auto notifier = uring->getNotifier();
  auto uring_worker = thread([uring = std::move(uring), prm = std::move(prm)] {
    try {
      const auto event = uring->awaitEvent(from(100ms));
      JMG_ENFORCE(pred(event), "timed out waiting for event");

      // send the user data back to the main thread
      prm->set_value(event.getUserData());
    }
    catch (...) {
      // propagate exception
      prm->set_exception(current_exception());
    }
  });

  io_uring msg_originator;
  io_uring_params params = {};

  // TODO(bd) are these flags correct?

  params.flags |= IORING_SETUP_SINGLE_ISSUER;
  params.flags |= IORING_SETUP_COOP_TASKRUN;
  params.flags |= IORING_SETUP_DEFER_TASKRUN;
  params.flags |= IORING_SETUP_SUBMIT_ALL;
  JMG_SYSTEM(io_uring_queue_init_params(256, &msg_originator, &params),
             "unable to initialize io_uring message originator");
  {
    // send some user data to the uring thread
    auto* sqe = io_uring_get_sqe(&msg_originator);
    JMG_ENFORCE(pred(sqe), "no submit queue entries available");
    io_uring_sqe_set_data64(sqe, 42);
    // don't trigger an event on the msg_originator uring when the message is
    // sent successfully
    io_uring_sqe_set_flags(sqe, IOSQE_CQE_SKIP_SUCCESS);
    // TODO(bd) figure out what flags get passed to this
    io_uring_prep_msg_ring(sqe, unsafe(notifier), 0 /*?*/, 0 /*?*/, 0 /*?*/);

    // fire off the message
    io_uring_submit(&msg_originator);

    // TODO(bd) check for success or error of the message?
  }

  // wait for the worker thread to shut down
  uring_worker.join();

  // grab the response from the future
  const auto user_data = ftr.get(1ms);
  EXPECT_EQ(42, unsafe(user_data));
}
#endif
