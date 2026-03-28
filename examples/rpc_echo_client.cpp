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

#include "jmg/cmdline.h"
#include "jmg/reactor/reactor_based_client.h"
#include "jmg/reactor/simple_rpc_client.h"

#include "rpc_echo_common.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace jmg::rpc_echo_svc
{

class RpcEchoClient : public ReactorBasedClient {
  using Client = RpcClient<EchoSvcIfce>;

  // command line argument
  using HostName =
    cmdline::NamedStringParam<"host",
                              "host to connect to (defaults to local host)",
                              Optional>;
  // TODO(bd) find a way to construct the usage string with the
  // default port number at compile time
  using Port = cmdline::
    NamedParam<IpPort, "port", "port to connect to (defaults to 7777)", Optional>;
  using CmdLine = cmdline::CmdLineArgs<HostName, Port>;

public:
  RpcEchoClient() = default;
  virtual ~RpcEchoClient() = default;

  void processArguments(const int argc, const char** argv) override {
    cout << "processing args\n";
    const auto cmdline = CmdLine(argc, argv);
    hostname_ = get<HostName>(cmdline, kDfltHostAddr);
    port_ = get<Port>(cmdline, kDfltPort);
    cout << "done processing args\n";
  }

  void execute(Reactor& reactor) override {
    Promise<string> work_product;
    reactor.execute([&](Fiber& fbr) {
      try {
        fbr.log("connecting to server at host [", hostname_, "] and port [",
                port_, "]");
        auto client = Client(fbr, IpEndpoint(hostname_, port_));

        EchoReq req;
        jmg::set<EchoStr>(req, "Hello RPC echo server!"s);

        fbr.log("executing query");
        const auto rsp = client.query(std::move(req));

        fbr.log("query complete");
        work_product.set_value(string(jmg::get<EchoStr>(rsp)));
        fbr.log("exiting reactor execution");
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
  IpPort port_;
};

JMG_REGISTER_CLIENT(RpcEchoClient);

} // namespace jmg::rpc_echo_svc

namespace jmg
{
JMG_REGISTER_CLIENT(rpc_echo_svc::RpcEchoClient);
} // namespace jmg
