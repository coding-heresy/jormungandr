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
#include <memory>
#include <string>

#include "jmg/cmdline.h"
#include "jmg/future.h"
#include "jmg/system.h"
#include "jmg/util.h"

#include "jmg/reactor/reactor_based_client.h"

#include "simple_tcp_service.h"

/**
 * quick and dirty reactor-based echo client
 */

using namespace std;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

namespace jmg
{

class ReactorBasedEchoClient : public ReactorBasedClient {
  // command line argument
  using HostName = NamedParam<string,
                              "host",
                              "host to connect to (defaults to local host)",
                              Optional>;
  using PortNum =
    NamedParam<uint16_t, "port", "port to connect to (defaults to 8888)", Optional>;
  using CmdLine = CmdLineArgs<HostName, PortNum>;

public:
  ReactorBasedEchoClient() = default;
  virtual ~ReactorBasedEchoClient() = default;

  void processArguments(const int argc, const char** argv) override {
    const auto cmdline = CmdLine(argc, argv);
    hostname_ = get_with_default<HostName>(cmdline, "127.0.0.1"s);
    port_ = Port(get_with_default<PortNum>(cmdline, 8888));
  }

  void execute(Reactor& reactor) override {
    Promise<string> work_product;
    reactor.execute([&](Fiber& fbr) {
      try {
        // connect to the server
        auto cnxn = SimpleTcpSvc::connectTo(fbr, IpEndpoint(hostname_, port_));
        const auto msg = string("Hello echo server!");

        // send the message
        cnxn.sendTo(buffer_from(msg));

        // receive the response
        const auto rsp = cnxn.rcvFrom();

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

    cout << "awaiting echoed data..." << endl;
    const auto msg =
      work_product.get_future().get(2s, "work completed awaiter");
    cout << "received echoed data [" << msg << "]\n";
  }

private:
  string hostname_;
  Port port_;
};

JMG_REGISTER_CLIENT(ReactorBasedEchoClient);

} // namespace jmg
