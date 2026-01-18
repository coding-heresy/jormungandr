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

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <ares.h>

#include "jmg/conversion.h"
#include "jmg/ip_endpoint.h"
#include "jmg/types.h"

#include "fiber.h"

namespace jmg
{

class DnsLookup {
public:
  using OptTimeout = std::optional<std::chrono::milliseconds>;

  static DnsLookup& instance();
  ~DnsLookup();

  template<NullTerminatedStringT Str, typename... Args>
  std::string lookup(Fiber& fbr, const Str& host, Args&&... args) {
    using namespace std::chrono;
    // convert any non-empty timeout value to milliseconds
    OptTimeout timeout;
    [[maybe_unused]]
    auto processArg = [&]<typename Arg>(Arg&& arg) {
      if constexpr (StdChronoDurationT<Arg>) {
        if constexpr (SameAsDecayedT<milliseconds, Arg>) { timeout = arg; }
        else { timeout = duration_cast<milliseconds>(arg); }
      }
      // TODO(bd) handle optional service name
      else { JMG_NOT_EXHAUSTIVE(Arg); }
    };
    (processArg(args), ...);
    return lookup(fbr, c_string_view(host), timeout);
  }

private:
  using AresAddrInfo = struct ::ares_addrinfo;
  using Channel = ::ares_channel_t;
  using LookupOpts = ::ares_options;
  using VTable = ::ares_socket_functions_ex;

  struct LookupResult {
    std::string addr;
    std::optional<Port> port = std::nullopt;
    std::exception_ptr exc_ptr = nullptr;
  };

  DnsLookup();

  /**
   * lookup implementation
   */
  std::string lookup(Fiber& fbr, c_string_view host, OptTimeout timeout);

  /**
   * construct a set of options to configure the lookup
   */
  static std::tuple<int, LookupOpts> makeLookupOpts(OptTimeout timeout);

  /**
   * construct a c-ares VTable that will use the reactor to execute
   * all syscalls required by the c-ares algorithm
   */
  static VTable makeSocketFcns();

  /**
   * handler called when the c-ares lookup is complete
   */
  static void aresCallback(void* user_data,
                           int status,
                           int timeouts,
                           AresAddrInfo* raw_result);
};

} // namespace jmg
