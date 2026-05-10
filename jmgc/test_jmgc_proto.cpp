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

#include "test_jmgc_any.h"

#include "jmg/protobuf/protobuf.h"
#include "test_jmgc.proto.h"

/**
 * test code specific to protobuf test
 */

namespace jmg::test_jmgc
{

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp) {
  using namespace jmg::test_jmgc;

  NonJmgTestMsg rslt;

  // boolean
  rslt.set_boolean(TestValues::kBoolean);

  // 32 bit integers
  rslt.set_int_32(TestValues::kInt32);
  rslt.set_uint_32(TestValues::kUInt32);
  rslt.set_sfixed_32(TestValues::kSFixed32);
  rslt.set_fixed_32(TestValues::kFixed32);

  // 64 bit integers
  rslt.set_int_64(TestValues::kInt64);
  rslt.set_uint_64(TestValues::kUInt64);
  rslt.set_sfixed_64(TestValues::kSFixed64);
  rslt.set_fixed_64(TestValues::kFixed64);

  // floating point
  rslt.set_flt(TestValues::kFlt);
  rslt.set_dbl(TestValues::kDbl);

  // strings
  rslt.set_str(TestValues::kStr);
  rslt.set_bytes_str(TestValues::kBytesStr);

  // time point
  {
    google::protobuf::Timestamp proto_ts = jmg::from(tp);
    *(rslt.mutable_time_stamp()) = std::move(proto_ts);
  }

  // safe integer ID
  rslt.set_int_id_fld(unsafe(TestValues::kIntId));

  // enum value
  rslt.set_active_state(TestValues::kActiveState);

  // primitive array
  for (const auto val : TestValues::kInts) { rslt.add_ints(val); }
  // string array
  for (const auto& val : TestValues::kStrs) { *(rslt.add_strs()) = val; }

  // object array
  {
    auto& inner = *(rslt.add_inner_msgs());
    inner.set_inner_int_32(TestValues::kInnerInts.at(0));
  }
  {
    auto& inner = *(rslt.add_inner_msgs());
    inner.set_inner_int_32(TestValues::kInnerInts.at(1));
    inner.set_opt_inner_str(*(TestValues::kInnerOptStrs.at(1)));
  }
  return rslt;
}

} // namespace jmg::test_jmgc
