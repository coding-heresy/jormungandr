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

#include <memory>

#include "jmg/cmdline.h"
#include "jmg/reactor/fiber.h"
#include "jmg/reactor/reactor.h"
#include "jmg/reactor/reactor_based_server.h"
#include "jmg/reactor/simple_rpc_service.h"
#include "jmg/server.h"

#include "rpc_echo_common.h"

using namespace jmg;
using namespace std;
using namespace std::chrono_literals;

namespace jmg::rpc_echo_svc
{

class RpcEchoServer : public ReactorBasedServer {
  using RpcEchoSvc = RpcSvc<EchoSvcIfce>;

  using Port = cmdline::
    NamedParam<IpPort, "port", "port to listen on (defaults to 7777)", Optional>;
  using CmdLine = cmdline::CmdLineArgs<Port>;

public:
  RpcEchoServer() = default;
  virtual ~RpcEchoServer() = default;

  void processArguments(const int argc, const char** argv) override final {
    const auto cmdline = CmdLine(argc, argv);
    port_ = get<Port>(cmdline, kDfltPort);
  }

  void startSrvr(Fiber& fbr) override final {
    try {
      fbr.log("creating listener endpoint using port [", port_, "]");
      svc_ =
        make_unique<RpcEchoSvc>(IpEndpoint("127.0.0.1", port_), is_shutdown_);

      auto handler = [](Fiber& handler_fbr, const EchoReq& req) -> EchoRsp {
        // very simple, create the response message and copy the request message
        // to it
        EchoRsp rsp;
        jmg::set<EchoStr>(rsp, jmg::get<EchoStr>(req));
        return rsp;
      };

      // NOTE: handleRequests blocks until the service is shut down
      svc_->handleRequests(fbr, std::move(handler));
    }
    JMG_SINK_ALL_EXCEPTIONS("accepting requests")
  }

  void shutdownSrvr() override final { svc_->shutdown(); }

private:
  IpPort port_;
  unique_ptr<RpcEchoSvc> svc_;
};

} // namespace jmg::rpc_echo_svc

namespace jmg
{
JMG_REGISTER_SERVER(rpc_echo_svc::RpcEchoServer);
} // namespace jmg
