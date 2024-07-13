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

#include <cxxabi.h>

#include "jmg/meta.h"

using namespace std;

namespace jmg
{

string demangle(const type_info& id) {
  // c.f.
  // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
  // for documentation of __cxa_demangle

  // note: -4 is not in the expected range of returned status values
  int status = -4;
  unique_ptr<char, void (*)(void*)> rslt{abi::__cxa_demangle(id.name(), nullptr,
                                                             nullptr, &status),
                                         free};
  if (!rslt) {
    switch (status) {
      case 0:
        throw runtime_error(
          "type name demangle returned success status but NULL result");
      case -1:
        throw runtime_error(
          "unable to allocate memory for demangled type name");
      case -2:
        throw runtime_error("unexpected invalid type name when demangling");
      case -3:
        throw runtime_error("invalid argument to typename demangling function");
      default:
        throw runtime_error(
          "unknown status result from failed typename demangle");
    }
  }
  // NOTE: ignoring the possibility that status could be nonzero when
  // __cxa_demangle() returns a non-NULL result
  return rslt.get();
}

string demangle(const type_info* id) { return demangle(*id); }

string current_exception_type_name() {
  const auto* excType = abi::__cxa_current_exception_type();
  if (!excType) { return "<no outstanding exceptions>"; }
  return demangle(excType);
}

} // namespace jmg
