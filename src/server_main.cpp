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

/**
 * standard main function for server application
 */

#include <csignal>

#include <array>
#include <iostream>
#include <thread>

#include "jmg/meta.h"
#include "jmg/preprocessor.h"
#include "jmg/server.h"
#include "jmg/system.h"
#include "jmg/util.h"

using namespace std;

namespace jmg
{
extern unique_ptr<Server> makeServer();
} // namespace jmg

using namespace jmg;

namespace
{

thread awaitShutdown(Server& srvr) {
  // standard set of shutdown signals for all servers
  static constexpr auto kSignals = array{
    SIGINT, // keyboard interrupt (i.e. ctrl-c)
    SIGTERM // external terminate request for server/daemon
  };
  return thread([&]() {
    try {
      const auto sig_set = makeSigSet(kSignals);
      int rcvd_signal;
      ////////////////////////////////////////
      // code blocks here until a signal is received
      ////////////////////////////////////////
      JMG_SYSTEM_ERRNO_RETURN(sigwait(&sig_set, &rcvd_signal),
                              "failed to wait on shutdown signals");
      cout << "received shutdown signal [" << rcvd_signal << "]" << endl;
    }
    // sink all exceptions here but always attempt to shut down the
    // server if unable to wait for the server
    JMG_SINK_ALL_EXCEPTIONS("signal handler thread")

    try {
      srvr.shutdown();
    }
    // sink any exceptions that might occur when attempting to shut
    // down the server
    JMG_SINK_ALL_EXCEPTIONS("server shutdown")
  });
}

} // namespace

int main(const int argc, const char** argv) {
  try {
    blockAllSignals();
    {
      auto server = makeServer();
      auto worker = awaitShutdown(*server);
      auto always_join_worker = Cleanup([&]() { worker.join(); });
      try {
        server->start(argc, argv);
      }
      catch (...) {
        // initiate shutdown if server startup fails
        send_shutdown_signal();
        // propagate the error up to the main handlers
        throw;
      }
    }
    return EXIT_SUCCESS;
  }
  JMG_SINK_ALL_EXCEPTIONS("top level")
  return EXIT_FAILURE;
}
