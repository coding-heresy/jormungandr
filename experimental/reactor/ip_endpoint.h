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

#include <netinet/in.h>

#include "jmg/safe_types.h"
#include "jmg/types.h"

namespace jmg
{

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using Port = SafeType<uint16_t, SafeIdType>;
#else
JMG_NEW_SAFE_TYPE(Port, uint16_t, SafeIdType);
#endif

JMG_DEFINE_RUNTIME_EXCEPTION(MalformedIpAddress);

/**
 * representation of an internet protocol endpoint, used for convenience and
 * improved type safety
 */
class IpEndpoint {
  static void makeSysAddr(const char* addr,
                          uint16_t port,
                          sockaddr_in& sys_addr);

public:
  template<NullTerminatedStringT T>
  IpEndpoint(const T& addr, Port port) {
    if constexpr (CStyleStringT<T>) {
      makeSysAddr(addr, unsafe(port), sys_addr_);
    }
    else {
      // should be either std::string or jmg::c_string_view
      makeSysAddr(addr.data(), unsafe(port), sys_addr_);
    }
  }

  const sockaddr_in& addr() const { return sys_addr_; }

private:
  sockaddr_in sys_addr_;
};

} // namespace jmg
