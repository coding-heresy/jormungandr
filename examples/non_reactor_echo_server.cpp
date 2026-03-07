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
 * non-reactor reference implementation of echo server for comparison
 * with experimental reactor versions
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/server.h"
#include "jmg/util.h"

using namespace std;
using namespace std::chrono_literals;

namespace jmg
{

class NonReactorEchoServer : public Server {
  static constexpr int kPort = 8888;

public:
  NonReactorEchoServer() = default;
  virtual ~NonReactorEchoServer() = default;

  void startImpl(const int argc, const char** argv) override final {
    ignore = argc;
    ignore = argv;
    is_shutdown_ = false;
    cout << "starting up...\n";
    try {
      sd_ = [&]() -> int {
        const auto rslt = ::socket(AF_INET, SOCK_STREAM, 0);
        JMG_SYSTEM(rslt, "creating socket");
        return rslt;
      }();
      {
        int opt = 1;
        JMG_SYSTEM(::setsockopt(sd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                &opt, sizeof(opt)),
                   "setting socket options");
      }

      {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(kPort);
        JMG_SYSTEM(::bind(sd_, (struct sockaddr*)&addr, sizeof(addr)),
                   "binding socket to port");
      }
      JMG_SYSTEM(::listen(sd_, SOMAXCONN), "listening to bound socket");
    }
    JMG_SINK_ALL_EXCEPTIONS("preparing listen socket")
    while (!is_shutdown_) {
      try {
        cout << "waiting for new connection...\n";
        // accept connection
        struct sockaddr_in addr;
        const auto cnxn_sd = [&]() -> int {
          int addr_sz = sizeof(addr);
          memset(&addr, 0, sizeof(addr));
          return ::accept(sd_, (struct sockaddr*)&addr, (socklen_t*)&addr_sz);
        }();
        if (is_shutdown_) {
          // exit the main accept loop
          break;
        }
        JMG_SYSTEM(cnxn_sd, "accepting connection");

        const auto closer = Cleanup([&]() {
          try {
            if (!uncaught_exceptions()) {
              // give the connection time to finish sending the data before
              // closing it
              this_thread::sleep_for(10ms);
            }
            JMG_SYSTEM(::close(cnxn_sd), "closing connection");
          }
          JMG_SINK_ALL_EXCEPTIONS("cleaning up open connection")
        });

        // read data length
        auto sz = [&]() -> size_t {
          size_t buf;
          const auto rslt = ::read(cnxn_sd, &buf, sizeof(buf));
          JMG_SYSTEM(rslt, "reading length");
          JMG_ENFORCE(rslt == sizeof(buf),
                      "incorrect number of octets read for size, expected [",
                      sizeof(buf), "] but got [", rslt, "]");
          return buf;
        }();

        // read message
        string msg;
        msg.resize(sz);
        sz = [&]() -> size_t {
          const auto rslt = ::read(cnxn_sd, msg.data(), msg.size());
          JMG_SYSTEM(rslt, "reading message");
          return static_cast<size_t>(rslt);
        }();
        JMG_ENFORCE(sz == msg.size(),
                    "incorrect number of octets read for message, expected [",
                    msg.size(), "] but got [", sz, "]");
        cout << "received message to echo: [" << msg << "]\n";

        // send message length
        JMG_SYSTEM(write(cnxn_sd, &sz, sizeof(sz)), "echoing message length");

        // send message
        JMG_SYSTEM(write(cnxn_sd, msg.data(), msg.size()), "echoing message");
      }
      JMG_SINK_ALL_EXCEPTIONS("handling connection");
    }
  }

  void shutdownImpl() override final {
    cout << "shutting down...\n";
    is_shutdown_ = true;
    if (-1 != sd_) {
      ::shutdown(sd_, SHUT_RDWR);
      ::close(sd_);
    }
  }

private:
  atomic<bool> is_shutdown_;
  int sd_ = -1;
};

JMG_REGISTER_SERVER(NonReactorEchoServer);

} // namespace jmg
