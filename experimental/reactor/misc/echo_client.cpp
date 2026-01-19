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

#include "jmg/cmdline.h"
#include "jmg/future.h"
#include "jmg/system.h"
#include "jmg/util.h"

#include "jmg/reactor/reactor.h"

/**
 * quick and dirty reactor-based echo client
 */

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

using HostName =
  NamedParam<string, "host", "host to connect to (defaults to local host)", Optional>;
using PortNum =
  NamedParam<uint16_t, "port", "port to connect to (defaults to 8888)", Optional>;
using CmdLine = CmdLineArgs<HostName, PortNum>;

int main(const int argc, const char** argv) {
  try {
    // process arguments
    const auto cmdline = CmdLine(argc, argv);
    const auto hostname = get_with_default<HostName>(cmdline, "127.0.0.1"s);
    const auto port = Port(get_with_default<PortNum>(cmdline, 8888));

    // start reactor
    Reactor reactor;
    auto [reactor_start_signal, reactor_start_rcvr] = makeSignaller();
    auto reactor_worker = thread([&] {
      blockAllSignals();
      try {
        reactor_start_signal.set_value();
        reactor.start();
      }
      JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
    });
    const auto awaitShutdown = Cleanup([&] { reactor_worker.join(); });
    reactor_start_rcvr.get(2s, "reactor start signal");

    Promise<string> work_product;
    reactor.execute([&](Fiber& fbr) {
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
          // const auto tgt = IpEndpoint("127.0.0.1", Port(8888));
          const auto tgt = IpEndpoint(hostname, port);
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
