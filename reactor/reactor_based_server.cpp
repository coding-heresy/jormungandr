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

#include "reactor_based_server.h"

using namespace std;

namespace jmg
{
ReactorBasedServer::ReactorBasedServer() : is_shutdown_(false) {}

void ReactorBasedServer::startImpl(const int argc, const char** argv) {
  ////////////////////
  // delegate argument processing to subclass
  processArguments(argc, argv);

  cout << "starting up with PID [" << getpid() << "]...\n";

  // start reactor
  auto [reactor_start_signal, reactor_start_rcvr] = makeSignaller();
  auto reactor_worker = thread([&] {
    try {
      reactor_start_signal.set_value();
      reactor_.start();
    }
    JMG_SINK_ALL_EXCEPTIONS("reactor worker thread top level")
  });
  const auto joiner = Cleanup([&] {
    if (reactor_worker.joinable()) { reactor_worker.join(); }
  });
  // 2 seconds is infinity
  reactor_start_rcvr.get(2s, "reactor start signal");

  ////////////////////
  // execute subclass-specific server behavior
  reactor_.execute([this](Fiber& fbr) { this->startSrvr(fbr); });
}

void ReactorBasedServer::shutdownImpl() {
  cout << "shutting down...\n";
  is_shutdown_ = true;
  shutdownSrvr();
  reactor_.shutdown();
}

} // namespace jmg
