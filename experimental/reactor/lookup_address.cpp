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

/**
 * Simple command line program that takes a hostname argument and logs
 * the list of IP endpoints associated with it.
 *
 * Mostly intended as a testbed for DNS lookup support in the reactor.
 */

#include <chrono>
#include <iostream>

#include "jmg/cmdline.h"
#include "jmg/future.h"
#include "jmg/system.h"

#include "reactor.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;

using Signaller = Promise<void>;
using SignallerPtr = shared_ptr<Signaller>;

namespace
{
auto makeSignaller() {
  auto signaller = Signaller();
  auto rcvr = signaller.get_future();
  return make_tuple(std::move(signaller), std::move(rcvr));
}
} // namespace

using Hostname =
  PosnParam<string, "hostname", "host name to look up address for">;
using SvcName =
  PosnParam<string, "service", "service name to look up port for", Optional>;
using CmdLine = CmdLineArgs<Hostname, SvcName>;

int main(const int argc, const char** argv) {
  try {
    // process arguments
    const auto cmdline = CmdLine(argc, argv);
    const auto hostname = get<Hostname>(cmdline);
    const auto svc_name = try_get<SvcName>(cmdline);

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

    {
      // execute query
      const auto terminator = Cleanup([&] { reactor.shutdown(); });
      const auto endpoints =
        reactor.compute([&](Fiber& fbr) mutable -> Fiber::IpEndpoints {
          if (svc_name) {
            return fbr.lookupNetworkEndpoints(hostname, *svc_name);
          }
          else { return fbr.lookupNetworkEndpoints(hostname); }
        });
      const auto svc_msg =
        svc_name ? str_cat(" and service [", *svc_name, "]") : string();
      cout << "IP endpoints for host [" << hostname << "]";
      if (svc_name) { cout << " and service [" << *svc_name << "]"; }
      cout << ":\n";
      for (const auto& endpoint : endpoints) {
        cout << " - " << static_cast<string>(from(endpoint.addr())) << "\n";
      }
    }

    return EXIT_SUCCESS;
  }
  JMG_SINK_ALL_EXCEPTIONS("main top level")
}
