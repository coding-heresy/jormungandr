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
 * system utilities
 */

#include <sys/types.h>
#include <unistd.h>

#include <csignal>

#include "jmg/preprocessor.h"
#include "jmg/types.h"

namespace jmg
{

////////////////////////////////////////////////////////////////////////////////
// signals
////////////////////////////////////////////////////////////////////////////////

/**
 * create a POSIX sigset_t with a specific set of signals added
 */
template<size_t kSz>
sigset_t makeSigSet(const std::array<int, kSz>& signals) {
  sigset_t rslt;
  JMG_SYSTEM(sigemptyset(&rslt), "failed to clear signal set object");
  for (const auto sig : signals) {
    JMG_SYSTEM(sigaddset(&rslt, sig), "failed to add signal [", sig,
               "] to signal set");
  }
  return rslt;
}

/**
 * block a set of relevant signals to avoid having them be delivered to a
 * process/thread
 */
void blockAllSignals();

/**
 * deliver a SIGTERM signal to the current process
 *
 * NOTE: used to initiate a clean shutdown in abnormal circumstances
 */
inline void send_shutdown_signal() {
  JMG_SYSTEM(kill(getpid(), SIGTERM), "failed to send shutdown signal");
}

////////////////////////////////////////////////////////////////////////////////
// read/write data to descriptor
////////////////////////////////////////////////////////////////////////////////

/**
 * read all bytes that a buffer can hold from a file descriptor, throwing
 * exception if they cannot all be read in one call
 */
template<ReadableDescriptorT FD>
void read_all(const FD fd,
              const BufferProxy buf,
              const std::string_view description) {
  namespace vws = std::ranges::views;
  using namespace std::string_view_literals;
  int sz;
  JMG_SYSTEM((sz = ::read(unsafe(fd), buf.data(), buf.size())),
             "unable to read all data from ", description);
  JMG_ENFORCE(buf.size() == static_cast<size_t>(sz),
              "size mismatch reading from ", description,
              ", should have read [", buf.size(), "] but actually read [", sz,
              "]");
}

/**
 * write all bytes from a buffer to a file descriptor, throwing exception if
 * they cannot all be written in one call
 */
template<WritableDescriptorT FD>
void write_all(const FD fd,
               const BufferView buf,
               const std::string_view description) {
  namespace vws = std::ranges::views;
  using namespace std::string_view_literals;
  int sz;
  JMG_SYSTEM((sz = ::write(unsafe(fd), buf.data(), buf.size())),
             "unable to write all data to ", description);
  JMG_ENFORCE(buf.size() == static_cast<size_t>(sz),
              "size mismatch writing to ", description,
              ", should have written [", buf.size(), "] but actually wrote [",
              sz, "]");
}

////////////////////////////////////////////////////////////////////////////////
// miscellaneous
////////////////////////////////////////////////////////////////////////////////

/**
 * create a pipe, return safely typed endpoints
 */
std::tuple<PipeReadFd, PipeWriteFd> make_pipe();

} // namespace jmg
