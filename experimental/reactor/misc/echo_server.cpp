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

using namespace std;
using namespace std::chrono_literals;

namespace jmg
{

class EchoServer : public Server {
  using PortNum =
    NamedParam<Port, "port", "port to listen on (defaults to 8888)", Optional>;
  using CmdLine = CmdLineArgs<PortNum>;
  using AcceptRslt = std::tuple<SocketDescriptor, IpEndpoint>;

  static constexpr auto kDfltPort = Port(8888);

public:
  EchoServer() = default;
  virtual ~EchoServer() = default;

  void startImpl(const int argc, const char** argv) override final {
    const auto cmdline = CmdLine(argc, argv);
    const auto port = get<PortNum>(cmdline, kDfltPort);
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
    reactor_.execute([&](Fiber& fbr) {
      try {
        acceptor_ = fbr.openSocket();
        {
          const int opt = 1;
          fbr.setSocketOption(acceptor_, SOL_SOCKET,
                              SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        }
        fbr.bindSocketToIfce(acceptor_, port);
        fbr.enableListenSocket(acceptor_);
        while (!is_shutdown_) {
          // accept new connection and pass it to a new fiber
          auto accept_rslt = [&] mutable -> optional<AcceptRslt> {
            try {
              return fbr.acceptCnxn(acceptor_);
            }
            catch (...) {
              // ignore exceptions that occur after shutdown, most
              // likely it is due to the socket being forcibly closed
              // by the shutdown handler
              if (!is_shutdown_) { rethrow_exception(current_exception()); }
            }
            return nullopt;
          }();
          if (!accept_rslt) {
            JMG_ENFORCE_USING(
              logic_error, is_shutdown_,
              "got empty socket accept result when not shutting down");
            continue;
          }
          const auto [sd, peer] = *accept_rslt;
          fbr.spawn([sd = sd](Fiber& fbr) {
            try {
              const auto closer = Cleanup([&] { fbr.close(sd); });

              // read message length
              auto sz = [&] -> size_t {
                size_t buf;
                const auto rslt = fbr.read(sd, buffer_from(buf));
                JMG_ENFORCE(
                  rslt == sizeof(buf),
                  "incorrect number of octets read for size, expected [",
                  sizeof(buf), "] but got [", rslt, "]");
                return buf;
              }();

              // read message
              string msg;
              msg.resize(sz);
              sz = fbr.read(sd, buffer_from(msg));
              JMG_ENFORCE(
                sz == msg.size(),
                "incorrect number of octets read for message, expected [",
                msg.size(), "] but got [", sz, "]");
              cout << "received message to echo: [" << msg << "]\n";

              // write response message length
              {
                const auto rsp_sz = msg.size();
                sz = fbr.write(sd, buffer_from(rsp_sz));
                JMG_ENFORCE(
                  sz == sizeof(rsp_sz),
                  "incorrect number of octets written for message, expected [",
                  sizeof(rsp_sz), "] but got [", sz, "]");
              }

              // write response message
              sz = fbr.write(sd, buffer_from(msg));
              JMG_ENFORCE(
                sz == msg.size(),
                "incorrect number of octets written for message, expected [",
                msg.size(), "] but got [", sz, "]");
            }
            JMG_SINK_ALL_EXCEPTIONS("handling echo request")
          });
        }
      }
      JMG_SINK_ALL_EXCEPTIONS("accepting new connections")
    });
    if (reactor_worker.joinable()) { reactor_worker.join(); }
  }

  void shutdownImpl() override final {
    cout << "shutting down...\n";
    is_shutdown_ = true;
    if (kInvalidSocketDescriptor != acceptor_) {
      ::shutdown(unsafe(acceptor_), SHUT_RDWR);
      ::close(unsafe(acceptor_));
      reactor_.shutdown();
    }
  }

private:
  atomic<bool> is_shutdown_;
  Reactor reactor_;
  SocketDescriptor acceptor_ = kInvalidSocketDescriptor;
};

JMG_REGISTER_SERVER(EchoServer);

} // namespace jmg
