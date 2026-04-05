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

#include <exception>

#include <meta/meta.hpp>

#include "jmg/meta.h"
#include "jmg/object.h"
#include "jmg/reactor/fiber.h"
#include "jmg/reactor/simple_tcp_service.h"

#include "simple_rpc_common.h"

namespace jmg
{

/**
 * base class template for simple RPC service
 */
template<TypeListT Msgs>
class RpcSvc : SimpleTcpSvc {
  static_assert(Msgs::size() == 2,
                "must be 2 messages, 1 for request and 1 for response");
  using ReqMsg = meta::front<Msgs>;
  using RspMsg = meta::back<Msgs>;

  using HandlerFcn = std::function<RspMsg(Fiber&, const ReqMsg&)>;

public:
  virtual ~RpcSvc() = default;
  JMG_NON_COPYABLE(RpcSvc);
  JMG_NON_MOVABLE(RpcSvc);
  RpcSvc(const IpEndpoint& endpoint, const ShutdownFlag& is_shutdown)
    : endpoint_(endpoint), is_shutdown_(is_shutdown) {}

  void handleRequests(Fiber& fbr, HandlerFcn fcn) {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    auto listener = SimpleTcpSvc::listenAt(fbr, endpoint_, is_shutdown_);
    listener_sd_ = listener.listener();
    while (!is_shutdown_) {
      listener.acceptCnxn([&](Fiber& handler_fbr, Cnxn cnxn,
                              const IpEndpoint peer) mutable {
        try {
          MetaDataObj rsp_metadata;
          jmg::set<RspCode>(rsp_metadata, RspCodes::kUnknownFailureType);
          // TODO(bd) fix cbe::set to support string_view argument
          jmg::set<ErrMsg>(rsp_metadata, "unknown failure type"s);
          RspWrapper rsp;
          const auto buf = cnxn.rcvFrom();
          try {
            ////////////////////
            // unpack message from incoming payload
            const auto incoming =
              cbe::Deserializer<ReqWrapper>(buffer_from(buf)).deserialize();
            const auto payload = jmg::get<ReqPayload>(incoming);
            const auto req =
              cbe::Deserializer<ReqMsg>(buffer_from(payload)).deserialize();

            ////////////////////
            // call handler
            const auto rsp_msg = fcn(handler_fbr, req);

            ////////////////////
            // build outgoing payload
            std::array<uint8_t, 4096> output = {};
            const auto serialized =
              cbe::Serializer<RspMsg>(buffer_from(output)).serialize(rsp_msg);
            const auto rsp_msg_data =
              std::string_view(reinterpret_cast<const char*>(serialized.data()),
                               serialized.size());
            jmg::set<RspPayload>(rsp, rsp_msg_data);
            jmg::set<RspCode>(rsp_metadata, RspCodes::kSuccess);
            jmg::set<MetaData>(rsp, rsp_metadata);
          }
          // TODO(bd) add more error response codes?
          catch (std::exception& e) {
            jmg::set<ErrMsg>(
              rsp_metadata, str_cat("caught exception when handling message: [",
                                    e.what(), "]"));
          }
          catch (...) {
            jmg::set<ErrMsg>(rsp_metadata,
                             str_cat("caught unexpected exception type [",
                                     current_exception_type_name(),
                                     "] when handling message"));
          }
          // serialize and send response
          std::array<uint8_t, 4096> output = {};
          const auto serialized =
            cbe::Serializer<RspWrapper>(buffer_from(output)).serialize(rsp);
          cnxn.sendTo(serialized);
        }
        JMG_SINK_ALL_EXCEPTIONS("handling new connection")
      });
    }
  }

  void shutdown() {
    // shutdown the listener socket, if necessary
    if (kInvalidSocketDescriptor != listener_sd_) {
      ::shutdown(unsafe(listener_sd_), SHUT_RDWR);
      ::close(unsafe(listener_sd_));
    }
  }

private:
  IpEndpoint endpoint_;
  const ShutdownFlag& is_shutdown_;
  SocketDescriptor listener_sd_ = kInvalidSocketDescriptor;
};

} // namespace jmg
