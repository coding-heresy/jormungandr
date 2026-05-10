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

#include "jmg/cbe/cbe.h"
#include "test_jmgc.cbe.h"

/**
 * test code specific to CBE test
 */

namespace jmg::test_jmgc
{

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp) {
  using namespace std::string_literals;
  auto ints = std::vector<int32_t>{2011};
  auto strs = std::vector<std::string>{"foo"s, "bar"s, "blub"s};
  using OptStr = std::optional<std::string>;
  auto inner_msgs =
    std::vector<InnerMsg>{InnerMsg(std::make_tuple(1, OptStr(std::nullopt))),
                          InnerMsg(std::make_tuple(2, OptStr("blub"s)))};
  return std::make_tuple(TestValues::kBoolean, TestValues::kInt32,
                         TestValues::kUInt32, TestValues::kSFixed32,
                         TestValues::kFixed32, TestValues::kInt64,
                         TestValues::kUInt64, TestValues::kSFixed64,
                         TestValues::kFixed64, TestValues::kFlt,
                         TestValues::kDbl, TestValues::kStr,
                         TestValues::kBytesStr, tp, TestValues::kIntId,
                         TestValues::kActiveState, std::move(ints),
                         std::move(strs), std::move(inner_msgs));
}

} // namespace jmg::test_jmgc
