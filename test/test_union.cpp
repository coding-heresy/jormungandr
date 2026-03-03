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

#include <gtest/gtest.h>

#include "jmg/field.h"
#include "jmg/union.h"

using namespace jmg;
using namespace std;

TEST(UnionTests, TestMetafunctions) {
  using IntFld = FieldDef<int, "int", Required>;
  using DblFld = FieldDef<double, "dbl", Required>;

  using TestUnion = Union<IntFld, DblFld>;
  using Objects = TestUnion::objects;
  EXPECT_TRUE((
    SameAsDecayedT<int, decltype(std::get<0>(declval<VariantizeT<Objects>>()))>));
  EXPECT_TRUE((SameAsDecayedT<double, decltype(std::get<1>(
                                        declval<VariantizeT<Objects>>()))>));
  cout << type_name_for<TestUnion::objects>() << endl;
  cout << type_name_for<TestUnion::return_type>() << endl;
}

////////////////////////////////////////////////////////////////////////////////
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
////////////////////////////////////////////////////////////////////////////////

#include "jmg/util.h"

namespace tmp
{
using HostName = SafeIdStr<>;
using IpAddrStr = SafeIdStr<>;
using DnsSvc = SafeIdStr<>;
using IpPort = SafeId32<>;
using SvcName = SafeIdStr<>;

using Host = variant<HostName, IpAddrStr>;
using SvcAccess = variant<DnsSvc, IpPort>;
using IpEndpoint = tuple<Host, SvcAccess>;
using PeerEndpoint = variant<IpEndpoint, SvcName>;

string getIpEndpoint(const IpEndpoint& endpoint) {
  string rslt;
  const auto& host_cfg = std::get<Host>(endpoint);
  std::visit(
    [&](const auto& host) {
      using Type = decltype(host);
      if constexpr (SameAsDecayedT<HostName, Type>) {
        rslt =
          str_cat("look up IP address of host name [", host, "] using DNS\n");
      }
      else {
        JMG_ENFORCE_USING(logic_error, (SameAsDecayedT<IpAddrStr, Type>),
                          "encountered unexpected type [", type_name_for(host),
                          "] when processing host configuration");
        rslt = str_cat("use IP address [", host, "]\n");
      }
    },
    host_cfg);
  const auto& svc_access = std::get<SvcAccess>(endpoint);
  std::visit(
    [&](const auto& svc_port) {
      using Type = decltype(svc_port);
      if constexpr (SameAsDecayedT<IpPort, Type>) {
        str_append(rslt, "use port number [", svc_port, "]\n");
      }
      else {
        JMG_ENFORCE_USING(logic_error, (SameAsDecayedT<DnsSvc, Type>),
                          "encountered unexpected type [",
                          type_name_for(svc_port),
                          "] when processing service access configuration");
        str_append(rslt, "look up standard port number for service [", svc_port,
                   "]\n");
      }
    },
    svc_access);
  return rslt;
}

string getPeerEndpoint(const PeerEndpoint& cfg) {
  string rslt;
  std::visit(
    [&](const auto& endpoint) {
      using Endpoint = decltype(endpoint);
      if constexpr (SameAsDecayedT<SvcName, Endpoint>) {
        rslt = str_cat("use name service to look up IP address and port of [",
                       endpoint, "]\n");
      }
      else {
        JMG_ENFORCE_USING(logic_error, (SameAsDecayedT<IpEndpoint, Endpoint>),
                          "encountered unexpected type [",
                          type_name_for(endpoint),
                          "] when processing peer endpoint configuration");
        rslt = getIpEndpoint(endpoint);
      }
    },
    cfg);
  return rslt;
}

}; // namespace tmp

TEST(UnionTests, DbgVariantStyle) {
  const auto host_name = tmp::HostName("rpc_svc.my.com");
  const auto ip_addr = tmp::IpAddrStr("192.168.69.69");
  const auto port = tmp::IpPort(8888);
  const auto dns_svc = tmp::DnsSvc("https");
  const auto svc_name = tmp::SvcName("rpc_svc");

  const auto host_name_endpoint = tmp::IpEndpoint(host_name, port);
  const auto ip_addr_endpoint = tmp::IpEndpoint(ip_addr, dns_svc);

  const auto peer_from_host_name = tmp::PeerEndpoint(host_name_endpoint);
  const auto peer_from_ip_addr = tmp::PeerEndpoint(ip_addr_endpoint);
  const auto peer_from_svc_name = tmp::PeerEndpoint(svc_name);

  cout << "=====\ninstructions for peer from hostname:\n"
       << tmp::getPeerEndpoint(peer_from_host_name) << endl;

  cout << "=====\ninstructions for peer from IP address:\n"
       << tmp::getPeerEndpoint(peer_from_ip_addr) << endl;

  cout << "=====\ninstructions for peer from service name:\n"
       << tmp::getPeerEndpoint(peer_from_svc_name) << endl;
}
