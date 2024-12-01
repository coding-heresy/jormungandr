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

#include "jmg/server.h"
#include "jmg/preprocessor.h"
#include "jmg/system.h"

namespace jmg
{

Server::~Server() {
  if (!is_shutdown_initiated_.test_and_set() && is_started_.test()) {
    // attempting to destroy the server after it has started but
    // before it has been shut down is not supported, ensure that the
    // server process will shut down
    send_shutdown_signal();
  }
}

void Server::start(const int argc, const char** argv) {
  JMG_ENFORCE(!is_started_.test_and_set(),
              "attempted to start the server more than once");

  // delegate the startup to the implementation
  this->startImpl(argc, argv);

  // TODO should there be a check for shutdown initiation here?
}

void Server::shutdown() {
  if (is_shutdown_initiated_.test_and_set()) {
    // ignore all attempts to shutdown the server after the first one
    return;
  }

  // delegate shutdown to the implementation
  this->shutdownImpl();
}

bool Server::isShutdownInitiated() const {
  return is_shutdown_initiated_.test();
}

} // namespace jmg
