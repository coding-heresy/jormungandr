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
TEST(UringTests, MsgTest) {
  auto notifier_prm = make_unique<Promise<FileDescriptor>>();
  auto notifier_ftr = notifier_prm->get_future();
  auto user_data_prm = make_unique<Promise<uring::UserData>>();
  auto user_data_ftr = user_data_prm->get_future();
  auto uring_worker = thread([notifier_prm = std::move(notifier_prm),
                              user_data_prm = std::move(user_data_prm)] {
    try {
      auto uring = uring::Uring(uring::UringSz(256));
      notifier_prm->set_value(uring.getNotifier());
      const auto event = uring.awaitEvent(from(100ms));
      JMG_ENFORCE(pred(event), "timed out waiting for event");

      // send the user data back to the main thread
      user_data_prm->set_value(event.getUserData());
    }
    catch (...) {
      // propagate exception
      user_data_prm->set_exception(current_exception());
    }
  });

  // retrieve the notifier file descriptor once it is ready
  const auto notifier = notifier_ftr.get(10ms);

  io_uring msg_originator;
  io_uring_params params = {};

  // TODO(bd) are these flags correct/necessary/optimal?
  params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_COOP_TASKRUN
                 | IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_SUBMIT_ALL;
  JMG_SYSTEM(io_uring_queue_init_params(256, &msg_originator, &params),
             "unable to initialize io_uring message originator");
  {
    // send some user data to the uring thread
    auto* sqe = io_uring_get_sqe(&msg_originator);
    JMG_ENFORCE(pred(sqe), "no submit queue entries available");
    // don't trigger an event on the msg_originator uring when the message is
    // sent successfully (i.e. the message is fire and forget)
    io_uring_sqe_set_flags(sqe, IOSQE_CQE_SKIP_SUCCESS);
    io_uring_prep_msg_ring(sqe, unsafe(notifier), 0 /* length (not needed) */,
                           42 /* data */, 0 /* flags (always 0) */);

    // fire off the message
    io_uring_submit(&msg_originator);
  }

  // wait for the worker thread to shut down
  uring_worker.join();

  // grab the response from the future
  const auto user_data = user_data_ftr.get(10ms);
  EXPECT_EQ(42, unsafe(user_data));
}
