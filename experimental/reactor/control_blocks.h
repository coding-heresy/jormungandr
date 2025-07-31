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
#include "jmg/util.h"

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
    JMG_ENFORCE_USING(std::logic_error, id <= next_never_used_,
                      "requested block ID [", id,
                      "] is larger than the largest block ID ever allocated (",
                      counter_, "]");
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
      // calculate bucket and index for the entry pointed to by the free list pointer
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
      // next free entry corresponds to the next ID
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
  uint16_t counter_ = 0;

  Id free_ = Id(0);
};

/**
 * singly linked list implementation for control blocks that uses the block ID
 * and control block link fields instead of pointers
 */
template<typename T>
class CtrlBlockQueue {
public:
  using Block = typename ControlBlocks<T>::ControlBlock;
  CtrlBlockQueue() = delete;
  JMG_NON_COPYABLE(CtrlBlockQueue);
  JMG_NON_MOVEABLE(CtrlBlockQueue);

  CtrlBlockQueue(ControlBlocks<T>& ctrl_blocks) : ctrl_blocks_(ctrl_blocks) {}

  void enqueue(const Block& item) {
    const auto id = item.id;
    if (tail_) {
      auto& tail_block = ctrl_blocks_.getBlock(*tail_);
      tail_block.id = id;
      tail_ = id;
    }
    else {
      JMG_ENFORCE_USING(std::logic_error, pred(!head_),
                        "control block queue head is set but tail is not");
      JMG_ENFORCE_USING(
        std::logic_error, !counter_,
        "control block queue head and tail are not set but counter is not 0");
      tail_ = id;
      head_ = id;
    }
    ++counter_;
  }

  Block& dequeue() {
    JMG_ENFORCE_USING(std::logic_error, !empty(),
                      "attempted to dequeue an item from an empty queue");
    auto& rslt = ctrl_blocks_.getBlock(*head_);
    if (1 == size()) {
      head_ = std::nullopt;
      tail_ = std::nullopt;
    }
    else { head_ = rslt.id; }
    --counter_;
    return rslt;
  }

  size_t size() const noexcept { return counter_; }

  bool empty() const noexcept { return !counter_; }

private:
  using Id = CtrlBlockId;

  ControlBlocks<T>& ctrl_blocks_;
  size_t counter_ = 0;
  std::optional<Id> head_ = std::nullopt;
  std::optional<Id> tail_ = std::nullopt;
};

} // namespace jmg
