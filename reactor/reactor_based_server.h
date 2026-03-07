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
#pragma once

#include <atomic>

#include "jmg/server.h"

#include "jmg/reactor/fiber.h"
#include "jmg/reactor/reactor.h"

namespace jmg
{

class ReactorBasedServer : public Server {
public:
  JMG_NON_COPYABLE(ReactorBasedServer);
  JMG_NON_MOVEABLE(ReactorBasedServer);
  ReactorBasedServer();
  virtual ~ReactorBasedServer() = default;

  /**
   * override of Server::startImpl() that bootstraps the server to the
   * point where the reactor is running
   */
  void startImpl(const int argc, const char** argv) override final;

  /**
   * override of Server::shutdownImpl() that initiates the shutdown
   * sequence
   */
  void shutdownImpl() override final;

  /**
   * overridden version of this function will be called automatically
   * to process any incoming arguments
   */
  virtual void processArguments(const int argc, const char** argv) = 0;

  /**
   * overridden version of this function will be called automatically
   * to execute the main body of the server code
   */
  virtual void startSrvr(Fiber& fbr) = 0;

  /**
   * overridden version of this function will be called automatically
   * to execute any subclass-specific shutdown initiation before the
   * reactor is shut down
   */
  virtual void shutdownSrvr() = 0;

protected:
  std::atomic<bool> is_shutdown_;
  Reactor reactor_;
};

} // namespace jmg
