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

#if defined(JMG_TEST_PROTOBUF)
#error "attempting to test both cbe and protobuf"
#endif
#if defined(JMG_TEST_YAML)
#error "attempting to test both cbe and yaml"
#endif

#include "test_jmgc.cbe.h"

namespace jmg::test_jmgc
{

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using FldDefType = jmg::cbe::Field<T, kName, IsRequired, kFldId>;

template<jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using StrFldDefType = jmg::cbe::StringField<kName, IsRequired, kFldId>;

template<typename T, jmg::StrLiteral kName, jmg::TypeFlagT IsRequired, uint32_t kFldId>
using ArrayDefType = jmg::cbe::ArrayField<T, kName, IsRequired, kFldId>;

template<typename T, typename... Ts>
using ObjDefType = jmg::cbe::Object<T, Ts...>;

using NonJmgTestMsg = TestMsg::adapted_type;
using JmgTestMsg = TestMsg;

NonJmgTestMsg makeNonJmgTestMsg(const jmg::TimePoint tp);

using NonJmgTestOptMsg = TestOptMsg::adapted_type;
using JmgTestOptMsg = TestOptMsg;

} // namespace jmg::test_jmgc

using namespace jmg::cbe;
