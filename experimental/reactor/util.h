/** -*- mode: c++ -*-
 *
 * Copyright (C) 2025 Brian Davis
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

#include <unistd.h>

#include <span>

#include "jmg/preprocessor.h"
#include "jmg/types.h"

namespace jmg::detail
{

// TODO(bd) remove debugging output once the code is confirmed stable

// namespace
// {
// template<typename... Args>
// void dbgOut(Args&&... args) {
//   using namespace std;
// #if defined(ENABLE_REACTOR_DEBUGGING_OUTPUT)
//   cout << ">>>>> DBG ";
//   (cout << ... << args);
//   cout << endl;
// #else
//   ignore = make_tuple(std::forward<Args>(args)...);
// #endif
// }
// } // namespace

/**
 * create a pipe, return safely typed endpoints
 */
std::tuple<PipeReadFd, PipeWriteFd> make_pipe() {
  int pipe_fds[2];
  JMG_SYSTEM(pipe(pipe_fds), "unable to create a pipe");
  return std::make_tuple(PipeReadFd(pipe_fds[0]), PipeWriteFd(pipe_fds[1]));
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
  // dbgOut("writing [", buf.size(), "] raw octets [",
  //        str_join(buf | vws::transform(octetify), " "sv, kOctetFmt),
  //        "] to file descriptor [", fd, "]");
  JMG_SYSTEM((sz = write(unsafe(fd), buf.data(), buf.size())),
             "unable to write all data to ", description);
  JMG_ENFORCE(buf.size() == static_cast<size_t>(sz),
              "size mismatch writing to ", description,
              ", should have written [", buf.size(), "] but actually wrote [",
              sz, "]");
}

/**
 * read all bytes that a buffer can hold from a file descriptor, throwing
 * exception if they cannot all be read in one call
 */
template<ReadableDescriptorT FD>
void read_all(const FD fd,
              const BufferProxy buf,
              const std::string_view description) {
  // dbgOut("reading [", buf.size(), "] octets of data from file descriptor [",
  // fd,
  //        "]");
  namespace vws = std::ranges::views;
  using namespace std::string_view_literals;
  int sz;
  JMG_SYSTEM((sz = read(unsafe(fd), buf.data(), buf.size())),
             "unable to read all data from ", description);
  JMG_ENFORCE(buf.size() == static_cast<size_t>(sz),
              "size mismatch reading from ", description,
              ", should have read [", buf.size(), "] but actually read [", sz,
              "]");
}
} // namespace jmg::detail
