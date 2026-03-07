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

#include <gmock/gmock.h>

#include "jmg/conversion.h"
#include "jmg/future.h"
#include "jmg/system.h"
#include "jmg/types.h"
#include "jmg/util.h"

#include "uring.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_literals;

TEST(UringTests, SmokeTest) {
  constexpr auto user_data = uring::UserData(42);
  uring::Uring uring(uring::UringSz(256));
  UringTimeSpec ts = from(Duration(10ms));
  uring.submitTimerEventReq(ts, user_data);
  const auto begin_ts = getCurrentTime();
  const auto event = uring.awaitEvent(from(100ms));
  EXPECT_TRUE(pred(event)) << "timed out waiting for event that should have "
                              "occurred before the timeout";
  const auto end_ts = getCurrentTime();
  EXPECT_EQ(unsafe(user_data), unsafe(event.getUserData()));

  const auto diff = end_ts - begin_ts;
  Duration duration_diff = from(diff);
  EXPECT_TRUE(duration_diff.count() >= 10 * kNanosecPerMillisec)
    << "event duration was less than expected timeout";
}

TEST(UringTests, TestTimeoutOnAwaitEvent) {
  constexpr auto user_data = uring::UserData(42);
  uring::Uring uring(uring::UringSz(256));
  UringTimeSpec ts = from(Duration(100ms));
  uring.submitTimerEventReq(ts, user_data);
  const auto event = uring.awaitEvent(from(10ms));
  // NOTE: awaitEvent should time out and an empty event should be returned
  EXPECT_FALSE(pred(event));
}

// test the ability to send messages to a uring owned by a worker thread via an
// eventfd notifier registered with the uring
TEST(UringTests, TestCrossUringMsg) {
  auto sync_prm = make_unique<Promise<void>>();
  auto sync_ftr = sync_prm->get_future();
  auto user_data_prm = make_unique<Promise<uint64_t>>();
  auto user_data_ftr = user_data_prm->get_future();
  const auto notifier = [&] {
    // store and forward
    int fd;
    JMG_SYSTEM((fd = eventfd(0 /* initval */, EFD_NONBLOCK)),
               "unable to create eventfd");
    return EventFd(fd);
  }();
  auto uring_worker = thread([notifier, sync_prm = std::move(sync_prm),
                              user_data_prm = std::move(user_data_prm)] {
    try {
      auto uring = uring::Uring(uring::UringSz(256));
      uring.registerEventNotifier(notifier);
      // signal the other thread to trigger the event
      sync_prm->set_value();
      const auto event = uring.awaitEvent(from(100ms));
      JMG_ENFORCE(pred(event), "timed out waiting for event");

      JMG_ENFORCE(static_cast<uint64_t>(unsafe(notifier))
                    == unsafe(event.getUserData()),
                  "incoming event did reference notifier as expected");

      uint64_t data;
      read_all(notifier, buffer_from(data), "eventfd"sv);

      // send the user data back to the main thread
      user_data_prm->set_value(data);
    }
    catch (...) {
      // propagate exception
      user_data_prm->set_exception(current_exception());
    }
  });

  // wait until the uring is ready for the event
  sync_ftr.get();

  // write the data
  const uint64_t event_data = 42;
  // store and forward
  {
    int sz;
    JMG_ENFORCE((sz = write(unsafe(notifier), &event_data, sizeof(event_data))),
                "unable to write event data to eventfd");
    JMG_ENFORCE(sizeof(event_data) == sz,
                "write size mismatch, should have written [",
                sizeof(event_data), "] but actually wrote [", sz, "]");
  }

  // wait for the worker thread to shut down
  uring_worker.join();

  // grab the response from the future
  const auto user_data = user_data_ftr.get(10ms);
  EXPECT_EQ(42, user_data);
}

TEST(UringTests, TestWrite) {
  uring::Uring uring(uring::UringSz(256));
  array<struct iovec, 1> io_array;
  constexpr auto kMsg = "logged to stdout\n"sv;
  io_array[0].iov_base = as_void_ptr(kMsg.data());
  io_array[0].iov_len = kMsg.size();
  uring.submitWriteReq(kStdoutFd, span(io_array));
  const auto event = uring.awaitEvent(from(100ms));
  JMG_ENFORCE(pred(event), "timed out waiting for event");
  if (event->res < 0) { JMG_THROW_SYSTEM_ERROR(-event->res); }
  EXPECT_EQ(kMsg.size(), static_cast<size_t>(event->res));
}

// TODO(bd) test that moving an instance of uring::Event does not break its
// destructor
