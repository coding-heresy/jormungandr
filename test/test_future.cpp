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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "jmg/future.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;

TEST(FutureTests, SmokeTest) {
  Promise<int> prm;
  auto ftr = prm.get_future();
  auto thr = thread([&] { prm.set_value(42); });
  auto val = ftr.get();
  EXPECT_EQ(42, val);
  thr.join();
}

TEST(FutureTests, VoidTest) {
  Promise<void> prm;
  auto ftr = prm.get_future();
  auto thr = thread([&] { prm.set_value(); });
  ftr.get();
  thr.join();
}

TEST(FutureTests, NoTimeoutTest) {
  Promise<void> prm;
  auto ftr = prm.get_future();
  auto thr = thread([&] { prm.set_value(); });
  ftr.get(1s);
  thr.join();
}

TEST(FutureTests, TimeoutTest) {
  Promise<void> prm;
  auto ftr = prm.get_future();
  EXPECT_THROW(ftr.get(10ms), runtime_error);
}

TEST(FutureTests, TimeoutExceptionTest) {
  Promise<void> prm;
  auto ftr = prm.get_future();
  try {
    ftr.get(10ms, "blocked future");
  }
  catch (const std::exception& e) {
    EXPECT_TRUE(string_view(e.what()).contains("blocked future"));
  }
}
