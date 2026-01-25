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

JMG_DEFINE_RUNTIME_EXCEPTION(AcceptInterrupted);

namespace jmg
{

/**
 * very simple service protocol that communicates over TCP and sends
 * messages consisting of an 8 octet header for the length followed by
 * the body
 */
class SimpleTcpSvc {
public:
  using ShutdownFlag = std::atomic<bool>;

  /**
   * owner of a socket associated with a live TCP connection
   */
  class Cnxn {
  public:
    JMG_NON_COPYABLE(Cnxn);
    // JMG_DEFAULT_MOVEABLE(Cnxn);
    Cnxn(Cnxn&& src);
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

    Fiber* fbr_ = nullptr;
    SocketDescriptor sd_;
  };

  /**
   * owner of a socket that can accept TCP connections from other
   * hosts
   */
  class CnxnAccepter {
  public:
    using AcceptHandler =
      std::function<void(Fiber&, Cnxn cnxn, IpEndpoint peer)>;

    JMG_DEFAULT_COPYABLE(CnxnAccepter);
    JMG_DEFAULT_MOVEABLE(CnxnAccepter);
    ~CnxnAccepter() = default;

    /**
     * await incoming connection requests and respond to each by
     * accepting the connection and spawning a new fiber to execute
     * the provided handler
     */
    void acceptCnxn(AcceptHandler&& fcn);

    /**
     * return the descriptor associated with the listen socket
     */
    SocketDescriptor listener() const noexcept { return sd_; }

  private:
    friend class SimpleTcpSvc;

    CnxnAccepter(Fiber& fbr,
                 SocketDescriptor sd,
                 const ShutdownFlag& is_shutdown);

    Fiber* fbr_ = nullptr;
    SocketDescriptor sd_;
    const ShutdownFlag* is_shutdown_ = nullptr;
  };

  /**
   * create a connection to an endpoint in the context of a reactor
   * fiber
   */
  static Cnxn connectTo(Fiber& fbr, const IpEndpoint& endpoint);

  // TODO(bd) create connectTo that can be called from outside
  // reactor?

  /**
   * create an object that can accept connections from other hosts
   */
  static CnxnAccepter listenAt(Fiber& fbr,
                               const IpEndpoint& endpoint,
                               const ShutdownFlag& is_shutdown);
};

} // namespace jmg
