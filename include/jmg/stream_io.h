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

#include <concepts>
#include <optional>
#include <span>

#include "jmg/types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// OutputStreamBuffer
////////////////////////////////////////////////////////////////////////////////

/**
 * class that provides a stream of buffers to which data can be
 * written
 *
 * somewhat inspired by the implementation that the protobuf C++
 * interface uses to implement serialization
 */
class OutputStreamBuffer {
public:
  explicit OutputStreamBuffer(std::span<Octet> buf);

  std::span<Octet> getNextChunk(size_t sz);

  void consume(std::optional<size_t> sz = std::nullopt);

  size_t remaining() const;

  size_t consumed() const;

private:
  // index of the the first octet in the buffer that has not been requested
  size_t idx_ = 0;
  // index of the the first octet in the buffer that has not been consumed
  size_t consumed_ = 0;
  std::span<Octet> buf_;
};

template<typename T>
concept ChunkedOutputStreamT =
  requires(T strm, size_t sz, std::optional<size_t> opt_sz) {
    // T::getNextChunk takes a size and returns a chunk
    { strm.getNextChunk(sz) } -> std::convertible_to<std::span<Octet>>;

    // T::consume takes an optional size and returns void
    { strm.consume(opt_sz) } -> std::same_as<void>;
  };

////////////////////////////////////////////////////////////////////////////////
// OutputStrmItr
////////////////////////////////////////////////////////////////////////////////

/**
 * sentinel type for OutputStrmItr
 */
struct OutputEnd {};

/**
 * class that wraps an octet buffer and provides an iterator to write to
 */
class OutputStrmItr {
public:
  // output iterator requirements
  using iterator_concept = std::output_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Octet;
  using pointer = Octet*;
  using reference = Octet&;

  explicit OutputStrmItr(OctetBufferProxy buf);

  reference operator*() const;

  pointer operator->() const;

  OutputStrmItr& operator++();

  bool operator==(const OutputStrmItr& other) const = default;

  bool operator==(OutputEnd) const;

  size_t remaining() const;

private:
  Octet* ptr_;
  Octet* end_;
};

////////////////////////////////////////////////////////////////////////////////
// InputStrmItr
////////////////////////////////////////////////////////////////////////////////

/**
 * sentinel type for InputStrmItr
 */
struct InputEnd {};

/**
 * class that wraps an octet buffer and provides an iterator to write to
 */
class InputStrmItr {
public:
  // input iterator requirements
  using iterator_concept = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Octet;
  using pointer = const Octet*;
  using reference = const Octet&;

  explicit InputStrmItr(OctetBufferView buf);

  reference operator*() const;

  pointer operator->() const;

  InputStrmItr& operator++();

  bool operator==(const InputStrmItr& other) const = default;

  bool operator==(InputEnd) const;

  size_t remaining() const;

private:
  const Octet* ptr_;
  const Octet* end_;
};

} // namespace jmg
