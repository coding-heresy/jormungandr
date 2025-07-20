/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "jmg/future.h"
#include "jmg/util.h"

#include "reactor.h"

using namespace jmg;
using namespace std;

TEST(ReactorTests, SmokeTest) {
  using PromiseOwner = unique_ptr<Promise<void>>;
  Reactor reactor;
  PromiseOwner shutdown_signaller = make_unique<Promise<void>>();
  auto shutdown_barrier = shutdown_signaller->get_future();
  thread reactor_worker([&reactor,
                         signaller = std::move(shutdown_signaller)] mutable {
    try {
      [&reactor, signaller = std::move(signaller)] mutable {
        reactor.start();
        signaller->set_value();
      }();
    }
    JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
  });

  try {
    // TODO(bd) handle startup timing better in the case where the
    // reactor starts at idle and is then instructed to perform work

    // wait until the reactor is actually running before shutting down
    this_thread::sleep_for(100ms);
    reactor.shutdown();
    shutdown_barrier.get(2s, "shutdown barrier");
    reactor_worker.join();
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
}

#if 0
// TODO(bd) current doesn't work
TEST(ReactorTests, TestSignalShutdown) {
  Reactor reactor;
  Promise<void> shutdown_signaller;
  thread reactor_worker([&] {
    try {
      reactor.start();
      shutdown_signaller.set_value();
    }
    JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
  });

  try {
    // TODO(bd) handle startup timing better in the case where the
    // reactor starts at idle and is then instructed to perform work

    // wait until the reactor is actually running before sending work
    this_thread::sleep_for(100ms);
    Promise<void> fbr_executed_signaller;
    reactor.post([&](Fiber&) {
      cout << "executing fiber" << endl;
      fbr_executed_signaller.set_value();
      cout << "done executing fiber" << endl;
    });
    cout << "waiting for posted work to complete" << endl;
    auto fbr_executed_barrier = fbr_executed_signaller.get_future();
    fbr_executed_barrier.get(2s, "fiber executed barrier");
    cout << "posted work completed, shutting down" << endl;
    reactor.shutdown();
    auto shutdown_barrier = shutdown_signaller.get_future();
    shutdown_barrier.get(2s, "shutdown barrier");
    reactor_worker.join();
    return;
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
  // should only get here if there was an exception
  if (reactor_worker.joinable()) {
    reactor_worker.join();
  }
}
#endif
