/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
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

#include <chrono>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>

#include "jmg/future.h"
#include "jmg/system.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

TEST(SystemTests, ThreadNameTest) {
  const auto name_str = "test_thread"s;
  const auto other_name_str = "other_thread"s;

  const auto old_thread_name = getThreadName();
  // ensure that the default name for the current thread is not "test_thread"
  EXPECT_NE(name_str, string_view(old_thread_name));
  setThreadName(name_str);
  EXPECT_EQ(name_str, getThreadName());

  Promise<void> notifier;
  Promise<string> other_thread_name;
  auto other_thread = thread([&] {
    auto notification = notifier.get_future();
    notification.get(10ms, "notification"sv);
    other_thread_name.set_value(getThreadName());
  });
  const auto cleanup = Cleanup([&] { other_thread.join(); });
  setThreadName(other_name_str, &other_thread);
  const auto expected = getThreadName(&other_thread);
  notifier.set_value();
  EXPECT_EQ(expected, other_thread_name.get_future().get());
}
