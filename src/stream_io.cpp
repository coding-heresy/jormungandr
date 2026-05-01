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

#include <exception>

#include "jmg/preprocessor.h"

using namespace std;

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// OutputStreamBuffer
////////////////////////////////////////////////////////////////////////////////

OutputStreamBuffer::OutputStreamBuffer(span<Octet> buf)
  : idx_(0), consumed_(0), buf_(buf) {}

span<Octet> OutputStreamBuffer::getNextChunk(const size_t sz) {
  JMG_ENFORCE_USING(logic_error, idx_ >= consumed_,
                    "internal corruption: index of consumed octets [",
                    consumed_, "] was higher than index of requested octets [",
                    idx_, "]");
  JMG_ENFORCE_USING(logic_error, consumed_ == idx_,
                    "previous chunk was not entirely consumed, there are [",
                    (idx_ - consumed_), "] octets remaining");
  const auto new_idx = idx_ + sz;
  JMG_ENFORCE_USING(logic_error, new_idx < buf_.size(),
                    "request exceeds available octets, total requested is [",
                    new_idx + 1, "] but total available is [", buf_.size(),
                    "]");
  const auto old_idx = idx_;
  idx_ = new_idx;
  return buf_.subspan(old_idx, sz);
}

void OutputStreamBuffer::consume(const optional<size_t> sz) {
  if (sz) {
    const auto new_consumed = consumed_ + *sz;
    JMG_ENFORCE_USING(logic_error, new_consumed <= idx_,
                      "attempted to consume more octets than were previously "
                      "requested, excess is [",
                      (new_consumed - idx_), "] octets");
    consumed_ = new_consumed;
    return;
  }
  // default is to consume all previously requested octets
  consumed_ = idx_;
}

size_t OutputStreamBuffer::remaining() const { return buf_.size() - idx_; }

size_t OutputStreamBuffer::consumed() const { return consumed_; }

////////////////////////////////////////////////////////////////////////////////
// OutputStrmItr
////////////////////////////////////////////////////////////////////////////////

OutputStrmItr::OutputStrmItr(OctetBufferProxy buf)
  : ptr_(buf.data()), end_(buf.data() + buf.size()) {
  JMG_ENFORCE(
    ptr_,
    "attempted to create an output stream iterator using an invalid buffer");
}

OutputStrmItr::reference OutputStrmItr::operator*() const {
  JMG_ENFORCE(ptr_ != end_, "attempted to dereference buffer end");
  return *ptr_;
}

OutputStrmItr::pointer OutputStrmItr::operator->() const {
  JMG_ENFORCE(ptr_ != end_, "attempted to dereference buffer end");
  return ptr_;
}

OutputStrmItr& OutputStrmItr::operator++() {
  JMG_ENFORCE(ptr_ != end_, "attempted to increment past buffer end");
  ++ptr_;
  return *this;
}

bool OutputStrmItr::operator==(OutputEnd) const { return ptr_ == end_; }

size_t OutputStrmItr::remaining() const { return distance(ptr_, end_); }

////////////////////////////////////////////////////////////////////////////////
// InputStrmItr
////////////////////////////////////////////////////////////////////////////////

InputStrmItr::InputStrmItr(OctetBufferView buf)
  : ptr_(buf.data()), end_(buf.data() + buf.size()) {
  JMG_ENFORCE(
    ptr_,
    "attempted to create an input stream iterator using an invalid buffer");
}

InputStrmItr::reference InputStrmItr::operator*() const {
  JMG_ENFORCE(ptr_ != end_, "attempted to dereference buffer end");
  return *ptr_;
}

InputStrmItr::pointer InputStrmItr::operator->() const {
  JMG_ENFORCE(ptr_ != end_, "attempted to dereference buffer end");
  return ptr_;
}

InputStrmItr& InputStrmItr::operator++() {
  JMG_ENFORCE(ptr_ != end_, "attempted to increment past buffer end");
  ++ptr_;
  return *this;
}

bool InputStrmItr::operator==(InputEnd) const { return ptr_ == end_; }

size_t InputStrmItr::remaining() const { return distance(ptr_, end_); }

} // namespace jmg
