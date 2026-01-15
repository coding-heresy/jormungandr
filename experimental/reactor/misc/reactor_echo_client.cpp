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

#include <iostream>
#include <string>

#include "jmg/future.h"
#include "jmg/system.h"
#include "jmg/util.h"

#include "reactor.h"

/**
 * quick and dirty reactor-based echo client
 */

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

int main(const int argc, const char** argv) {
  ignore = argc;
  ignore = argv;
  try {
    // start reactor in a separate thread
    Reactor reactor;
    auto reactor_worker = make_unique<thread>([&] {
      blockAllSignals();
      try {
        reactor.start();
      }
      JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
    });

    // set up RAII cleanup for reactor
    const auto joiner = Cleanup([&]() {
      try {
        cout << "joining reactor worker thread...\n";
        auto awaiter = Future(async(launch::async, [&]() -> bool {
          if (reactor_worker->joinable()) {
            reactor_worker->join();
            return true;
          }
          return false;
        }));
        const auto rslt =
          awaiter.get(2s, "waiting for reactor worker thread join"sv);
        if (!rslt) { cout << "reactor worker thread was not joinable\n"; }
        cout << "done joining reactor worker thread...\n";
      }
      JMG_SINK_ALL_EXCEPTIONS("reactor worker joiner")
    });

    Promise<string> work_product;
    reactor.post([&](Fiber& fbr) {
      try {
        // open socket
        const auto sd = fbr.openSocket(SocketTypes::kTcp);
        const auto closer = Cleanup([&]() {
          try {
            fbr.close(sd);
          }
          JMG_SINK_ALL_EXCEPTIONS("connection socket closer")
        });

        // connect to service
        {
          // NOTE: address lookup isn't supported yet so can only
          // connect to an IPv4 address
          const auto tgt = IpEndpoint("127.0.0.1", Port(8888));
          fbr.connectTo(sd, tgt);
        }

        const auto msg = string("Hello echo server!");

        // send the message size
        {
          const auto sz = msg.size();
          fbr.write(sd, buffer_from(sz));
        }

        // send the message
        fbr.write(sd, buffer_from(msg));

        // read the response
        string rsp;
        rsp.resize(msg.size());
        const auto sz = fbr.read(sd, buffer_from(rsp));

        // process the response
        JMG_ENFORCE(sz == rsp.size(), "expected [", sz,
                    "] octets in the response but received [", rsp.size(), "]");
        work_product.set_value(std::move(rsp));
      }
      catch (...) {
        try {
          const auto ptr = current_exception();
          if (!ptr) {
            work_product.set_value("failed, but no current exception!?!?");
          }
          work_product.set_exception(ptr);
        }
        JMG_SINK_ALL_EXCEPTIONS("fiber body exception handler")
      }
    });

    const auto msg =
      work_product.get_future().get(2s, "work completed awaiter");
    cout << "++++++++++ received echoed data [" << msg << "]\n";
    reactor.shutdown();
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
}
