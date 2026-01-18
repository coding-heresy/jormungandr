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

using namespace std;

namespace jmg
{

void IpEndpoint::makeSysAddr(const char* addr,
                             uint16_t port,
                             sockaddr_in& sys_addr) {
  JMG_ENFORCE_USING(MalformedIpAddress,
                    ctre::match<"^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$">(addr),
                    "provided address [", addr,
                    "] is not a correctly formatted IPv4 address");
  memset(&sys_addr, 0, sizeof(sys_addr));
  sys_addr.sin_family = AF_INET;
  JMG_SYSTEM(inet_pton(AF_INET, addr, &(sys_addr.sin_addr)),
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

} // namespace jmg
