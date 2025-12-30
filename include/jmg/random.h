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

#include <concepts>

#include <absl/random/random.h>

#include "jmg/preprocessor.h"

namespace jmg
{

/**
 * generate a random number in a range using a uniform distribution
 */
template<std::integral T>
class RandomInRange {
  using Distribution = absl::uniform_int_distribution<T>;

public:
  RandomInRange(const T rng_begin, const T rng_end)
    : distribution_([&]() -> Distribution {
      JMG_ENFORCE(rng_begin < rng_end, "bad range in constructor, end value [",
                  rng_end, "] is less than or equal to begin value [",
                  rng_begin, "]");
      return Distribution(rng_begin, rng_end);
    }()) {}

  /**
   * get a uniformly distributed random value in the range
   *
   * @warning: this function is not thread-safe
   */
  T get() { return distribution_(generator_); }

private:
  absl::BitGen generator_;
  absl::uniform_int_distribution<T> distribution_;
};

} // namespace jmg
