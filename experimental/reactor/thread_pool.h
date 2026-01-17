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

/**
 * Multiple 3rd party implementations of a thread pool wrapped with a
 * common interface to (hopefully) allow them to be dropped in to code
 * via a simple type alias
 */

#if defined(JMG_BOOST_THREAD_POOL_WORKS)
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#endif
#include <BS_thread_pool.hpp>

#include "jmg/preprocessor.h"

namespace jmg
{

#if defined(JMG_BOOST_THREAD_POOL_WORKS)
class BoostThreadPool {
public:
  BoostThreadPool(size_t thread_count = 1) : pool_(thread_count) {}
  JMG_NON_COPYABLE(BoostThreadPool);
  JMG_NON_MOVEABLE(BoostThreadPool);
  ~BoostThreadPool() = default;

  void join() { pool_.join(); }

  template<typename Fcn>
    requires std::invocable<Fcn>
  void execute(Fcn&& fcn) {
    boost::asio::post(pool_, std::forward<Fcn>(fcn));
  }

private:
  boost::asio::thread_pool pool_;
};
#endif

class BsThreadPool {
public:
  BsThreadPool(size_t thread_count = 1) : pool_(thread_count) {}
  JMG_NON_COPYABLE(BsThreadPool);
  JMG_NON_MOVEABLE(BsThreadPool);
  ~BsThreadPool() = default;

  void join() { pool_.wait(); }

  template<typename Fcn>
    requires std::invocable<Fcn>
  void execute(Fcn&& fcn) {
    pool_.detach_task(std::forward<Fcn>(fcn));
  }

private:
  // TODO(bd) support optional features?
  BS::thread_pool<BS::tp::none> pool_;
};

} // namespace jmg
