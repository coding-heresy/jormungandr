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

#include "control_blocks.h"

using namespace jmg;
using namespace std;

using TestControlBlocks = ControlBlocks<uint8_t>;
using Id = TestControlBlocks::Id;

TEST(ControlBlocksTests, SmokeTest) {
  TestControlBlocks ctrl;

  const auto [id0, block0] = ctrl.getOrAllocate();
  // ID 0 allocated from unused
  EXPECT_EQ(id0, Id(0));
  EXPECT_EQ(id0, block0.id);

  const auto [id1, block1] = ctrl.getOrAllocate();
  // ID 1 allocated from unused
  EXPECT_EQ(id1, Id(1));
  EXPECT_EQ(id1, block1.id);

  ctrl.release(id0);
  // ID 0 is now on the free stack

  const auto [id2, block2] = ctrl.getOrAllocate();
  // ID 0 allocated from the free stack
  EXPECT_EQ(id2, Id(0));
  EXPECT_EQ(id2, block2.id);

  const auto [id3, block3] = ctrl.getOrAllocate();
  // ID 2 allocated from unused
  EXPECT_EQ(id3, Id(2));
  EXPECT_EQ(id3, block3.id);

  ctrl.release(id1);
  // ID 1 is now on the free stack

  ctrl.release(id3);
  // ID 2 is now on the free stack

  const auto [id4, block4] = ctrl.getOrAllocate();
  // ID 2 allocated from the free stack
  EXPECT_EQ(id4, Id(2));
  EXPECT_EQ(id4, block4.id);
}

TEST(ControlBlocksTests, TestErrorCases) {
  TestControlBlocks ctrl;

  // try to release ID that has never been used
  EXPECT_THROW(ctrl.release(Id(0)), logic_error);

  const auto [id, block] = ctrl.getOrAllocate();
  ctrl.release(id);

  // try to release ID that is on the free stack
  EXPECT_THROW(ctrl.release(id), logic_error);
}
