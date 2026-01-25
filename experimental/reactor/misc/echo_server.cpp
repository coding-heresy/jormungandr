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

#include <chrono>

#include "jmg/cmdline.h"
#include "jmg/server.h"

#include "jmg/reactor/fiber.h"
#include "jmg/reactor/reactor.h"
#include "jmg/reactor/simple_tcp_service.h"

using namespace std;
using namespace std::chrono_literals;

namespace jmg
{

class EchoServer : public Server {
  using Port =
    NamedParam<IpPort, "port", "port to listen on (defaults to 8888)", Optional>;
  using CmdLine = CmdLineArgs<Port>;
  using AcceptRslt = std::tuple<SocketDescriptor, IpEndpoint>;

  static constexpr auto kDfltPort = IpPort(8888);

public:
  EchoServer() = default;
  virtual ~EchoServer() = default;

  void startImpl(const int argc, const char** argv) override final {
    const auto cmdline = CmdLine(argc, argv);
    const auto port = get<Port>(cmdline, kDfltPort);
    cout << "starting up with PID [" << getpid() << "]...\n";

    auto [reactor_start_signal, reactor_start_rcvr] = makeSignaller();
    auto reactor_worker = thread([&] {
      try {
        reactor_start_signal.set_value();
        reactor_.start();
      }
      JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
    });
    // 2 seconds is infinity
    reactor_start_rcvr.get(2s, "reactor start signal");
    reactor_.execute([this, port = port](Fiber& fbr) {
      cout << "fiber [" << fbr.getId()
           << "] executing listener task using port [" << port << "]\n";
      executeFbrTask(fbr, port);
    });
    if (reactor_worker.joinable()) { reactor_worker.join(); }
  }

  void shutdownImpl() override final {
    cout << "shutting down...\n";
    is_shutdown_ = true;
    if (kInvalidSocketDescriptor != listener_sd_) {
      ::shutdown(unsafe(listener_sd_), SHUT_RDWR);
      ::close(unsafe(listener_sd_));
      reactor_.shutdown();
    }
  }

private:
  using Cnxn = SimpleTcpSvc::Cnxn;
  using CnxnAccepter = SimpleTcpSvc::CnxnAccepter;

  /**
   * actual work is done here
   */
  void executeFbrTask(Fiber& fbr, const IpPort port) {
    try {
      // create the listener
      cout << "fiber [" << fbr.getId()
           << "] creating listener endpoint using port [" << port << "]\n";
      const auto listen_endpoint = IpEndpoint("127.0.0.1", port);
      auto listener =
        SimpleTcpSvc::listenAt(fbr, listen_endpoint, is_shutdown_);
      listener_sd_ = listener.listener();
      while (!is_shutdown_) {
        cout << "fiber [" << fbr.getId() << "] awaiting next connection\n";
        listener.acceptCnxn([&](Fiber& fbr, Cnxn cnxn,
                                const IpEndpoint peer) mutable {
          try {
            cout << "fiber [" << fbr.getId() << "] connected to peer at ["
                 << peer.str() << "]\n";
            const auto msg = cnxn.rcvFrom();
            cout << "fiber [" << fbr.getId() << "] received message to echo: ["
                 << msg << "]\n";
            cnxn.sendTo(buffer_from(msg));
            cout << "fiber [" << fbr.getId() << "] finished echoing message\n";
          }
          JMG_SINK_ALL_EXCEPTIONS("handling echo request")
        });
      }
    }
    JMG_SINK_ALL_EXCEPTIONS("accepting new connections")
  }

  atomic<bool> is_shutdown_;
  Reactor reactor_;
  SocketDescriptor listener_sd_ = kInvalidSocketDescriptor;
};

JMG_REGISTER_SERVER(EchoServer);

} // namespace jmg
