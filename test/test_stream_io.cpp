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

#include "jmg/stream_io.h"

#include <array>

#include <gtest/gtest.h>

#include "jmg/types.h"
#include "jmg/util.h"

using namespace jmg;
using namespace std;

inline span<char> spanify(span<Octet> octets) {
  return span(reinterpret_cast<char*>(octets.data()), octets.size());
}

TEST(StreamIoTests, SmokeTest) {
  constexpr size_t kSz = 10;
  auto buffer = array<Octet, kSz>();
  memset(buffer.data(), '\0', buffer.size());
  auto buf_strm = OutputStreamBuffer(span(buffer));
  EXPECT_EQ(0UL, buf_strm.consumed());
  EXPECT_EQ(kSz, buf_strm.remaining());

  size_t consumed = 0;
  size_t remaining = kSz;

  {
    const auto chunk = buf_strm.getNextChunk(1);
    remaining -= 1;
    EXPECT_EQ(consumed, buf_strm.consumed());
    EXPECT_EQ(remaining, buf_strm.remaining());

    chunk[0] = octetify('f');
  }

  buf_strm.consume();
  consumed += 1;
  EXPECT_EQ(consumed, buf_strm.consumed());
  EXPECT_EQ(remaining, buf_strm.remaining());

  {
    const auto chunk = buf_strm.getNextChunk(2);
    remaining -= 2;
    EXPECT_EQ(consumed, buf_strm.consumed());
    EXPECT_EQ(remaining, buf_strm.remaining());

    chunk[0] = octetify('o');
    buf_strm.consume(1);
    consumed += 1;

    EXPECT_EQ(consumed, buf_strm.consumed());
    EXPECT_EQ(remaining, buf_strm.remaining());

    chunk[1] = octetify('o');
    buf_strm.consume();
    consumed += 1;

    EXPECT_EQ(consumed, buf_strm.consumed());
    EXPECT_EQ(remaining, buf_strm.remaining());
  }
}
