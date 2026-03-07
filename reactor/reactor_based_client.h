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

#include "jmg/future.h"
#include "jmg/preprocessor.h"
#include "jmg/system.h"

#include "jmg/reactor/reactor.h"

namespace jmg
{

class ReactorBasedClient {
public:
  JMG_NON_COPYABLE(ReactorBasedClient);
  JMG_NON_MOVEABLE(ReactorBasedClient);
  ReactorBasedClient() = default;
  virtual ~ReactorBasedClient() = default;

  /**
   * client subclass must override this function to change the number
   * of worker threads in the reactor thread pool
   */
  virtual size_t reactorWorkerThreadCount() const noexcept { return 1; }

  /**
   * overridden version of this function will be called automatically
   * to process any incoming arguments
   */
  virtual void processArguments(const int argc, const char** argv) = 0;

  /**
   * overridden version of this function will be called automatically
   * once the reactor has been started
   */
  virtual void execute(Reactor& reactor) = 0;
};

} // namespace jmg

#define JMG_REGISTER_CLIENT(type)                         \
  std::unique_ptr<jmg::ReactorBasedClient> makeClient() { \
    return std::make_unique<type>();                      \
  }
