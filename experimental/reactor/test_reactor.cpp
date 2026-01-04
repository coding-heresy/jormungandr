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

#include <atomic>
#include <future>
#include <stacktrace>
#include <thread>

#include <gtest/gtest.h>

#include "jmg/file_util.h"
#include "jmg/future.h"
#include "jmg/system.h"
#include "jmg/util.h"

#include "reactor.h"

using namespace jmg;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// test fixture
////////////////////////////////////////////////////////////////////////////////

class ReactorTests : public ::testing::Test {
protected:
  void SetUp() override {
    reactor_worker = make_unique<thread>([&] {
      blockAllSignals();
      try {
        reactor.start();
        clean_reactor_shutdown = true;
      }
      JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
    });
    // TODO(bd) handle startup timing better in the case where the
    // reactor starts at idle and is then instructed to perform work

    // wait until the reactor is actually running before shutting down
    this_thread::sleep_for(100ms);
  }
  void TearDown() override {
    // try to join the worker thread, just in case...
    if (reactor_worker && reactor_worker->joinable()) {
      reactor_worker->join();
    }
  }

  Reactor reactor;
  unique_ptr<thread> reactor_worker;
  atomic<bool> clean_reactor_shutdown = false;
};

using Signaller = Promise<void>;

////////////////////////////////////////////////////////////////////////////////
// test cases
////////////////////////////////////////////////////////////////////////////////

TEST_F(ReactorTests, SmokeTest) {
  // shut down the reactor immediately
  reactor.shutdown();
  reactor_worker->join();
  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestSignalShutdown) {
  // request execution of a fiber function that will signal when it runs
  Signaller fbr_executed_signaller;
  reactor.post([&](Fiber&) { fbr_executed_signaller.set_value(); });

  // wait until the fiber function completes before proceeding
  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() {
      // shutdown the reactor after the fiber functions have executed
      reactor.shutdown();
      if (!uncaught_exceptions()) {
        JMG_ENFORCE(reactor_worker, "reactor worker thread does not exist");
        JMG_ENFORCE(reactor_worker->joinable(),
                    "reactor worker is not joinable");
      }
      reactor_worker->join();
    });
    // 2 seconds is infinity
    fbr_executed_barrier.get(2s, "fiber executed barrier");
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestFiberYielding) {
  auto fbr_executed_signaller1 = make_shared<Signaller>();
  auto fbr_executed_signaller2 = make_shared<Signaller>();

  using SignallerPtr = shared_ptr<Signaller>;
  auto make_fbr_fcn = [](SignallerPtr&& signaller) mutable {
    return [signaller = std::move(signaller)](Fiber& fbr) mutable {
      cout << "starting work for fiber [" << fbr.getId() << "]" << endl;
      for (int step : views::iota(1, 3)) {
        cout << "fiber [" << fbr.getId() << "] will now yield at step [" << step
             << "]" << endl;
        fbr.yield();
        cout << str_cat("fiber [", fbr.getId(),
                        "] has finished yielding at step [", step, "]")
             << endl;
      }
      signaller->set_value();
    };
  };

  auto fbr_executed_barrier1 = fbr_executed_signaller1->get_future();
  auto fbr_executed_barrier2 = fbr_executed_signaller2->get_future();

  auto fbr_fcn1 = make_fbr_fcn(std::move(fbr_executed_signaller1));
  reactor.post(std::move(fbr_fcn1));
  reactor.post(make_fbr_fcn(std::move(fbr_executed_signaller2)));

  {
    auto guard = Cleanup([&]() {
      // shutdown the reactor after the fiber functions have executed
      reactor.shutdown();
      if (!uncaught_exceptions()) {
        JMG_ENFORCE(reactor_worker, "reactor worker thread does not exist");
        JMG_ENFORCE(reactor_worker->joinable(),
                    "reactor worker is not joinable");
      }
      reactor_worker->join();
    });
    // wait until the fiber functions complete before proceeding
    // 2 seconds is infinity
    fbr_executed_barrier1.get(2s, "fiber executed barrier 1");
    fbr_executed_barrier2.get(2s, "fiber executed barrier 2");
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestFileOpenFailure) {
  Signaller fbr_executed_signaller;
  reactor.post([&](Fiber& fbr) {
    cout << "executing 'open_file' operation that should fail\n";

    EXPECT_THROW([[maybe_unused]] auto fd =
                   fbr.openFile("/no/such/file", FileOpenFlags::kRead),
                 system_error);

    cout << "done executing 'open file' operation that should fail\n";
    fbr_executed_signaller.set_value();
  });

  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() {
      // shutdown the reactor after the fiber functions have executed
      cout << "shutting down the reactor\n";
      reactor.shutdown();
      if (!uncaught_exceptions()) {
        JMG_ENFORCE(reactor_worker, "reactor worker thread does not exist");
        JMG_ENFORCE(reactor_worker->joinable(),
                    "reactor worker is not joinable");
      }
      reactor_worker->join();
    });
    cout << "awaiting completion of reactor operation\n";
    // 2 seconds is infinity
    fbr_executed_barrier.get(2s, "fiber executed barrier");
    cout << "reactor operation complete\n";
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestWriteDataToFile) {
  const auto test_data = "some test data"s;
  TmpFile tmpFile;
  Signaller fbr_executed_signaller;
  reactor.post([&](Fiber& fbr) {
    cout << "opening temporary file [" << tmpFile.name() << "] for writing\n";
    const auto fd = fbr.openFile(tmpFile.name(), FileOpenFlags::kWrite, 0644);
    {
      auto guard = Cleanup([&]() { fbr.close(fd); });

      cout << "executing 'write' operation on temporary file ["
           << tmpFile.name() << "]\n";
      fbr.write(fd, buffer_from(test_data));

      cout << "done writing data to temporary file [" << tmpFile.name()
           << "]\n";
    }
    fbr_executed_signaller.set_value();
  });

  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() {
      // shutdown the reactor after the fiber functions have executed
      cout << "shutting down the reactor\n";
      reactor.shutdown();
      if (!uncaught_exceptions()) {
        JMG_ENFORCE(reactor_worker, "reactor worker thread does not exist");
        JMG_ENFORCE(reactor_worker->joinable(),
                    "reactor worker is not joinable");
      }
      reactor_worker->join();
    });
    cout << "awaiting completion of fiber work\n";
    // 2 seconds is infinity
    fbr_executed_barrier.get(2s, "fiber executed barrier");

    {
      // validate the data written to the file
      const auto sz = filesystem::file_size(tmpFile.path());
      EXPECT_EQ(sz, test_data.size());
      auto file_data = [&]() -> string {
        auto strm = open_file<ifstream>(tmpFile.path());
        string rslt;
        rslt.resize(sz);
        strm.read(rslt.data(), static_cast<std::streamsize>(sz));
        return rslt;
      }();
      EXPECT_EQ(file_data, test_data);
    }
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}
