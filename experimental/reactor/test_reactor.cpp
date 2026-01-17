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

  /**
   * helper function to shut down the reactor when a test is complete
   */
  void shutdown() {
    reactor.shutdown();
    if (!uncaught_exceptions()) {
      JMG_ENFORCE(reactor_worker, "reactor worker thread does not exist");
      JMG_ENFORCE(reactor_worker->joinable(), "reactor worker is not joinable");
    }
    reactor_worker->join();
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
    auto guard = Cleanup([&]() { shutdown(); });
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
      for (int step : views::iota(1, 3)) {
        fbr.log("fiber [", fbr.getId(), "] is yielding at step [", step, "]\n");
        fbr.yield();
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
    EXPECT_THROW([[maybe_unused]] auto fd =
                   fbr.openFile("/no/such/file", FileOpenFlags::kRead),
                 system_error);
    fbr_executed_signaller.set_value();
  });

  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() { shutdown(); });
    // 2 seconds is infinity
    fbr_executed_barrier.get(2s, "fiber executed barrier");
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestReadDataFromFile) {
  const auto test_data = "some test data"s;
  TmpFile tmpFile(test_data);
  Signaller fbr_executed_signaller;
  string file_data;
  file_data.resize(test_data.size() + 1);
  size_t read_sz = 0;
  reactor.post([&](Fiber& fbr) mutable {
    const auto fd = fbr.openFile(tmpFile.name(), FileOpenFlags::kRead);
    {
      auto guard = Cleanup([&]() { fbr.close(fd); });
      read_sz = fbr.read(fd, buffer_from(file_data));
    }
    fbr_executed_signaller.set_value();
  });

  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() { shutdown(); });
    // 2 seconds is infinity
    fbr_executed_barrier.get(2s, "fiber executed barrier");

    {
      // validate the data read from the file
      EXPECT_EQ(read_sz, test_data.size());
      file_data.resize(read_sz);
      EXPECT_EQ(file_data, test_data);
    }
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestWriteDataToFile) {
  const auto test_data = "some test data"s;
  TmpFile tmpFile;
  Signaller fbr_executed_signaller;
  reactor.post([&](Fiber& fbr) {
    const auto fd = fbr.openFile(tmpFile.name(), FileOpenFlags::kWrite, 0644);
    {
      auto guard = Cleanup([&]() { fbr.close(fd); });
      fbr.write(fd, buffer_from(test_data));
    }
    fbr_executed_signaller.set_value();
  });

  auto fbr_executed_barrier = fbr_executed_signaller.get_future();
  {
    auto guard = Cleanup([&]() { shutdown(); });
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

TEST_F(ReactorTests, TestThreadPoolExecution) {
  {
    auto guard = Cleanup([&]() { shutdown(); });
    auto reactor_thread_id_prm = make_shared<Promise<thread::id>>();
    auto pool_thread_id_prm = make_shared<Promise<thread::id>>();
    auto reactor_thread_id = reactor_thread_id_prm->get_future();
    auto pool_thread_id = pool_thread_id_prm->get_future();
    // post work to a fiber in the reactor
    reactor.post([reactor_thread_id_prm = std::move(reactor_thread_id_prm),
                  pool_thread_id_prm =
                    std::move(pool_thread_id_prm)](Fiber& fbr) mutable {
      string reactor_thread_id = from(this_thread::get_id());
      // report the thread ID of the thread running the reactor
      reactor_thread_id_prm->set_value(this_thread::get_id());
      // forward the work to the thread pool
      fbr.execute([pool_thread_id_prm =
                     std::move(pool_thread_id_prm)]() mutable {
        this_thread::sleep_for(10ms);
        string pool_thread_id = from(this_thread::get_id());
        // report the thread ID of the thread in the thread pool executing this work
        pool_thread_id_prm->set_value(this_thread::get_id());
      });
      this_thread::sleep_for(10ms);
    });

    // 2 seconds is infinity
    const auto reactor_id = reactor_thread_id.get(2s, "reactor thread ID");
    const auto pool_id = pool_thread_id.get(2s, "pool thread ID");
    EXPECT_NE(reactor_id, pool_id);
    EXPECT_NE(this_thread::get_id(), reactor_id);
    EXPECT_NE(this_thread::get_id(), pool_id);
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestThreadPoolComputation) {
  {
    auto guard = Cleanup([&]() { shutdown(); });
    auto rslt_val_prm = Promise<double>();
    auto rslt_val = rslt_val_prm.get_future();
    reactor.post([&](Fiber& fbr) {
      auto sqrtr = [](const double val) { return sqrt(val); };
      auto rslt = fbr.compute(std::move(sqrtr), 4.0);
      rslt_val_prm.set_value(rslt);
    });
    EXPECT_NEAR(2.0, rslt_val.get(), 1e-6);
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}

TEST_F(ReactorTests, TestThreadPoolComputationFailurePropagatesToFiber) {
  {
    auto guard = Cleanup([&]() { shutdown(); });
    auto rslt_val_prm = Promise<bool>();
    auto rslt_val = rslt_val_prm.get_future();
    reactor.post([&](Fiber& fbr) {
      auto thrower = []([[maybe_unused]] const double val) -> double {
        throw runtime_error("not really exceptional");
      };
      try {
        [[maybe_unused]] auto rslt = fbr.compute(std::move(thrower), 4.0);
      }
      catch (const exception& exc) {
        rslt_val_prm.set_value(true);
        return;
      }
      // should only reach this point if the computation didn't throw
      // exception as expected
      rslt_val_prm.set_value(false);
    });
    EXPECT_TRUE(rslt_val.get());
  }

  EXPECT_TRUE(clean_reactor_shutdown);
}
