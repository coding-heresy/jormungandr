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

#include "simple_tcp_service.h"

using namespace std;

namespace jmg
{

SimpleTcpSvc::Cnxn::Cnxn(Fiber& fbr, SocketDescriptor sd)
  : fbr_(&fbr), sd_(sd) {}

SimpleTcpSvc::Cnxn::Cnxn(Cnxn&& src) {
  this->fbr_ = src.fbr_;
  this->sd_ = src.sd_;
  src.fbr_ = nullptr;
  src.sd_ = kInvalidSocketDescriptor;
}

SimpleTcpSvc::Cnxn::~Cnxn() {
  if (fbr_ && (kInvalidSocketDescriptor != sd_)) { fbr_->close(sd_); }
}

void SimpleTcpSvc::Cnxn::sendTo(BufferView msg) {
  const auto sz = msg.size();
  fbr_->write(sd_, buffer_from(sz));
  fbr_->write(sd_, msg);
}

std::string SimpleTcpSvc::Cnxn::rcvFrom() {
  size_t msg_sz;
  auto sz = fbr_->read(sd_, buffer_from(msg_sz));
  // TODO(bd) return empty message on 0 bytes to indicate
  // connection closed?
  JMG_ENFORCE(sz == sizeof(msg_sz),
              "failed to read incoming message header, expected [",
              sizeof(msg_sz), "] octets but received [", sz, "]");
  std::string msg;
  msg.resize(msg_sz);
  sz = fbr_->read(sd_, buffer_from(msg));
  JMG_ENFORCE(sz == msg_sz, "failed to read incoming message, expected [",
              msg_sz, "] octets but received [", sz, "]");
  return msg;
}

SimpleTcpSvc::CnxnAccepter::CnxnAccepter(Fiber& fbr,
                                         const SocketDescriptor sd,
                                         const ShutdownFlag& is_shutdown)
  : fbr_(&fbr), sd_(sd), is_shutdown_(&is_shutdown) {}

void SimpleTcpSvc::CnxnAccepter::acceptCnxn(AcceptHandler&& fcn) {
  try {
    auto [sd, peer] = fbr_->acceptCnxn(sd_);
    fbr_->spawn([fcn = std::move(fcn), sd = sd, peer = peer](Fiber& fbr) {
      try {
        fcn(fbr, Cnxn(fbr, sd), peer);
      }
      JMG_SINK_ALL_EXCEPTIONS("handling accepted connection")
    });
  }
  catch (...) {
    if (is_shutdown_) {
      // TODO(bd) validate that this is the correct exception type
      throw AcceptInterrupted(str_cat("accept call in fiber [", fbr_->getId(),
                                      "] was interrupted"));
    }
    rethrow_exception(current_exception());
  }
}

SimpleTcpSvc::Cnxn SimpleTcpSvc::connectTo(Fiber& fbr,
                                           const IpEndpoint& endpoint) {
  const auto sd = fbr.openSocket(SocketTypes::kTcp);
  auto cleaner = Cleanup([&]() { fbr.close(sd); });
  fbr.connectTo(sd, endpoint);
  std::move(cleaner).cancel();
  return Cnxn(fbr, sd);
}

SimpleTcpSvc::CnxnAccepter SimpleTcpSvc::listenAt(
  Fiber& fbr,
  const IpEndpoint& endpoint,
  const ShutdownFlag& is_shutdown) {
  const auto sd = fbr.openSocket();
  {
    const int opt = 1;
    fbr.setSocketOption(sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                        sizeof(opt));
  }
  fbr.bindSocketToIfce(sd, endpoint.port());
  fbr.enableListenSocket(sd);
  return CnxnAccepter(fbr, sd, is_shutdown);
}

} // namespace jmg
