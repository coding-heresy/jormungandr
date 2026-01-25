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

#include "jmg/ip_endpoint.h"

#include <arpa/inet.h>

#include <ctre.hpp>

#include "jmg/preprocessor.h"
#include "jmg/util.h"

using namespace std;

namespace jmg
{

IpV4Addr::IpV4Addr(const std::string_view src) : addr_str_(src) {
  JMG_ENFORCE_USING(MalformedIpAddress,
                    ctre::match<"^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$">(src),
                    "provided address [", src,
                    "] is not a correctly formatted IPv4 address");
}

IpV4Addr::IpV4Addr(const struct sockaddr_in& src) {
  char buf[INET_ADDRSTRLEN + 1];
  memset(buf, 0, sizeof(buf));
  JMG_SYSFCN_PTR_RETURN(::inet_ntop(AF_INET, &(src.sin_addr), buf,
                                    INET_ADDRSTRLEN),
                        "converting socket address struct to string address");
  addr_str_ = buf;
}

void IpEndpoint::makeSysAddr(const char* addr,
                             uint16_t port,
                             sockaddr_in& sys_addr) {
  JMG_ENFORCE_USING(MalformedIpAddress,
                    ctre::match<"^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$">(addr),
                    "provided address [", addr,
                    "] is not a correctly formatted IPv4 address");
  memset(&sys_addr, 0, sizeof(sys_addr));
  sys_addr.sin_family = AF_INET;
  JMG_SYSTEM(::inet_pton(AF_INET, addr, &(sys_addr.sin_addr)),
             "converting IP address to binary");
  sys_addr.sin_port = htons(port);
}

IpEndpoint::IpEndpoint(const sockaddr& addr, const optional<size_t> sz) {
  JMG_ENFORCE(AF_INET == addr.sa_family, "address family type [",
              addr.sa_family, "] is not currently supported");
  JMG_ENFORCE(!sz || (sz >= sizeof(sys_addr_)), "provided size [", *sz,
              "] is too small, must be at least [", sizeof(sys_addr_), "]");
  memset(&sys_addr_, 0, sizeof(sys_addr_));
  memcpy(&sys_addr_, &addr, sizeof(sys_addr_));
}

std::string_view IpEndpoint::str() const {
  if (!str_addr_) {
    const auto addr = IpV4Addr(sys_addr_);
    if (sys_addr_.sin_port != 0) {
      str_addr_ = str_cat(addr.str(), ":", sys_addr_.sin_port);
    }
    else { str_addr_ = addr.str(); }
  }
  return *str_addr_;
}

} // namespace jmg
