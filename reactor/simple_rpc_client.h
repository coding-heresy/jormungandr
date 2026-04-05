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

#include "jmg/ip_endpoint.h"
#include "jmg/meta.h"
#include "jmg/reactor/fiber.h"
#include "jmg/reactor/simple_tcp_service.h"

#include "simple_rpc_common.h"

namespace jmg
{

template<TypeListT Msgs>
class RpcClient {
  static_assert(Msgs::size() == 2,
                "must be 2 messages, 1 for request and 1 for response");
  using ReqMsg = meta::front<Msgs>;
  using RspMsg = meta::back<Msgs>;

public:
  virtual ~RpcClient() = default;
  JMG_NON_COPYABLE(RpcClient);
  JMG_NON_MOVABLE(RpcClient);
  RpcClient(Fiber& fbr, const IpEndpoint& tgt_endpoint)
    : fbr_(fbr), cnxn_(SimpleTcpSvc::connectTo(fbr, tgt_endpoint)) {}

  RspMsg query(ReqMsg&& req) {
    using namespace std::string_literals;

    ////////////////////
    // make request wrapper payload by serializing request message to raw bytes
    std::array<uint8_t, 4096> payload;
    auto outgoing = [&]() -> ReqWrapper {
      auto serializer = cbe::Serializer<ReqMsg>(buffer_from(payload));
      serializer.serialize(req);
      ReqWrapper outgoing;
      auto raw_data = std::string(reinterpret_cast<const char*>(payload.data()),
                                  serializer.consumed());
      jmg::set<ReqPayload>(outgoing, raw_data);
      return outgoing;
    }();

    ////////////////////
    // serialize request wrapper to buffer
    std::array<uint8_t, 8192> raw_req;
    auto serializer = cbe::Serializer<ReqWrapper>(buffer_from(raw_req));
    const auto serialized = serializer.serialize(outgoing);

    cnxn_.sendTo(serialized);

    const auto raw_rsp = cnxn_.rcvFrom();

    ////////////////////
    // deserialize response wrapper
    const auto incoming = [&] -> RspWrapper {
      auto deserializer = cbe::Deserializer<RspWrapper>(buffer_from(raw_rsp));
      return deserializer.deserialize();
    }();
    const auto& metadata = jmg::get<MetaData>(incoming);
    const auto rc = jmg::get<RspCode>(metadata);

    if (RspCodes::kSuccess != rc) {
      // throw exception for failed request
      const auto rsp_err_msg = jmg::try_get<ErrMsg>(metadata);
      const auto base_err_msg = [&]() -> std::string {
        switch (rc) {
          case RspCodes::kUnknownFailureType:
            return "target service was unable to process the request"s;
          default:
            return str_cat("received unknown response code [",
                           static_cast<std::underlying_type_t<RspCodes>>(rc),
                           "] from target service");
        }
      }();
      if (rsp_err_msg) {
        JMG_RUNTIME_ERROR(base_err_msg, ", error message was: ", *rsp_err_msg);
      }
      JMG_RUNTIME_ERROR(base_err_msg, ", no further information was provided");
    }

    ////////////////////
    // deserialize and return the response
    const auto raw_rsp_msg = jmg::try_get<RspPayload>(incoming);
    JMG_ENFORCE(pred(raw_rsp_msg),
                "no response payload was available in successful response");
    return cbe::Deserializer<RspMsg>(buffer_from(*raw_rsp_msg)).deserialize();
  }

private:
  Fiber& fbr_;
  SimpleTcpSvc::Cnxn cnxn_;
};

} // namespace jmg
