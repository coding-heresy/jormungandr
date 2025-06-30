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
#pragma once

/**
 * Low-level control structures inspired somewhat by the process control block
 * concept
 *
 * Uses a simple linked-list style implementation based on table indices instead
 * of pointers.
 */

#include <cstdint>
#include <cstring>

#include <limits>
#include <memory>
#include <tuple>
#include <vector>

#include "jmg/preprocessor.h"
#include "jmg/safe_types.h"

namespace jmg
{

#if defined(JMG_SAFETYPE_ALIAS_TEMPLATE_WORKS)
using CtrlBlockId = SafeType<uint16_t, SafeIdType, st::incrementable>;
#else
JMG_NEW_SAFE_TYPE(CtrlBlockId, uint16_t, SafeIdType, st::incrementable);
#endif

/**
 * table of control blocks, organized as 255 buckets of 255 blocks
 *
 * blocks that were previously allocated and then freed are organized into a
 * stack using block identifiers as the links
 */
template<typename T>
struct ControlBlocks {
public:
  using Id = CtrlBlockId;

  struct ControlBlock {
    T body;
    Id id;
    Id link;
  };

  ControlBlocks() {
    buckets_[0] = std::make_unique<Bucket>();
    memset(static_cast<void*>(buckets_[0].get()), 0, sizeof(Bucket));
    auto& block = getBlock(Id(0));
    block.link = kMax;
  }

  /**
   * get a reference to the block with a given identifier
   */
  ControlBlock& getBlock(const Id id) {
    const auto [bucket, idx] = decompose(id);
    return (*(buckets_[bucket]))[idx];
  }

  /**
   * get a previously allocated but currently unused block or allocate a new
   * one, if possible
   */
  std::tuple<Id, ControlBlock&> getOrAllocate() {
    JMG_ENFORCE(next_never_used_ < kMax, "control block table is full");
    JMG_ENFORCE_USING(std::logic_error, free_ <= next_never_used_,
                      "free stack pointer exceeds counter");
    const auto never_used = (free_ == next_never_used_);
    if (never_used) {
      const auto [bucket, idx] = decompose(free_);
      if ((bucket > 0) && (idx == 0)) {
        // allocate a new bucket
        buckets_[bucket] = std::make_unique<Bucket>();
        memset(static_cast<void*>(buckets_[bucket].get()), 0, sizeof(Bucket));
      }
    }
    const auto id = free_;
    auto& block = getBlock(id);
    block.id = id;
    auto& body = block.body;
    memset(static_cast<void*>(&body), 0, sizeof(body));

    if (never_used) {
      ++free_;
      ++next_never_used_;
    }
    else {
      if (kMax == block.link) {
        // free stack is now empty, now point to never used space
        free_ = next_never_used_;
      }
      else { free_ = block.link; }
    }

    block.link = kMax;
    ++counter_;
    return std::forward_as_tuple(id, block);
  }

  /**
   * release the block with the given ID to the free list
   */
  void release(const Id id) {
    JMG_ENFORCE_USING(std::logic_error, id < next_never_used_, "ID [", id,
                      "] was never allocated");
    auto& block = getBlock(id);
    JMG_ENFORCE_USING(std::logic_error, block.link == kMax,
                      "double release of ID [", id, "]");

    // push the block onto the free stack
    block.link = free_;
    free_ = id;
    --counter_;
  }

  /**
   * return the count of all blocks currently in use
   */
  uint16_t count() const { return counter_; }

private:
  using Bucket = std::array<ControlBlock, 255>;
  using Buckets = std::array<std::unique_ptr<Bucket>, 255>;

  static constexpr auto kMax = Id(std::numeric_limits<uint16_t>::max());

  /**
   * split control block ID into (bucket, index)
   */
  auto decompose(const Id id) {
    static constexpr uint16_t kMask = 255;
    const auto idx = static_cast<uint8_t>(unsafe(id) & kMask);
    const auto bucket = static_cast<uint8_t>(unsafe(id) >> 8);
    return std::tuple(bucket, idx);
  }

  Id next_never_used_ = Id(0);
  Buckets buckets_;
  Id free_ = Id(0);
  uint16_t counter_ = 0;
};

} // namespace jmg
