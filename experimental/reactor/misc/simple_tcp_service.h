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

#include <string>

#include "jmg/ip_endpoint.h"
#include "jmg/types.h"

#include "jmg/reactor/fiber.h"

namespace jmg
{

/**
 * very simple service protocol that communicates over TCP and sends
 * messages consisting of an 8 octet header for the length followed by
 * the body
 */
class SimpleTcpSvc {
public:
  class Cnxn {
  public:
    JMG_NON_COPYABLE(Cnxn);
    JMG_NON_MOVEABLE(Cnxn);
    ~Cnxn();

    /**
     * send a message to the peer
     */
    void sendTo(BufferView msg);

    /**
     * receive a message from the peer
     */
    std::string rcvFrom();

  private:
    friend class SimpleTcpSvc;

    Cnxn(Fiber& fbr, SocketDescriptor sd);

    Fiber& fbr_;
    SocketDescriptor sd_;
  };

  /**
   * create a connection to an endpoint in the context of a reactor
   * fiber
   */
  static Cnxn connectTo(Fiber& fbr, const IpEndpoint& endpoint);

  // TODO(bd) create connectTo that can be called from outside
  // reactor?

  // TODO(bd) create Listener class
};

} // namespace jmg
