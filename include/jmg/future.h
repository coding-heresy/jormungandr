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

/**
 * improved promise/future
 *
 * mostly, this involves allowing a future to time out and throw an exception when
 * calling get without having to deal with any std::future_status foolishness
 */

#include <future>
#include <optional>
#include <string_view>

#include "jmg/conversion.h"
#include "jmg/preprocessor.h"

namespace jmg
{

// TODO(bd) create a wrapper for std::shared_future as well?

/**
 * improved version of std::future that dispenses with std::future_status in the
 * external interface and allows the caller to specify a timeout (with optional
 * description) to the get() member function and throws exception if the timeout
 * is exceeded
 */
template<typename T>
class Future {
public:
  Future() = default;
  JMG_DEFAULT_MOVEABLE(Future);
  JMG_NON_COPYABLE(Future);

  Future(std::future<T>&& ftr) : ftr_(std::move(ftr)) {}

  auto get() { return ftr_.get(); }

  bool valid() const noexcept { return ftr_.valid(); }

  template<DurationT Dur8n>
  auto get(const Dur8n timeout,
           const std::optional<std::string_view> description = std::nullopt) {
    using namespace std::string_view_literals;
    const auto status = [&] -> std::future_status {
      if constexpr (StdChronoDurationT<Dur8n>) {
        return ftr_.wait_for(timeout);
      }
      else { return ftr_.wait_for(from(timeout)); }
    }();
    JMG_ENFORCE(std::future_status::timeout != status, "timed out waiting for ",
                (description ? *description : "future"sv));
    return ftr_.get();
  }

  // TODO(bd) add a version of get() with a timeout deadline instead of a duration?

private:
  std::future<T> ftr_;
};

/**
 * specialization for void
 */
template<>
class Future<void> {
public:
  Future() = default;
  JMG_DEFAULT_MOVEABLE(Future);
  JMG_NON_COPYABLE(Future);

  Future(std::future<void>&& ftr) : ftr_(std::move(ftr)) {}

  void get() { ftr_.get(); }

  bool valid() const noexcept { return ftr_.valid(); }

  template<DurationT Dur8n>
  void get(const Dur8n timeout,
           const std::optional<std::string_view> description = std::nullopt) {
    using namespace std::string_view_literals;
    const auto status = [&] -> std::future_status {
      if constexpr (StdChronoDurationT<Dur8n>) {
        return ftr_.wait_for(timeout);
      }
      else { return ftr_.wait_for(from(timeout)); }
    }();
    JMG_ENFORCE(std::future_status::timeout != status, "timed out waiting for ",
                (description ? *description : "future"sv));
    ftr_.get();
  }

  // TODO(bd) add a version of get() with a timeout deadline instead of a duration?

private:
  std::future<void> ftr_;
};

/**
 * this class does not differ from std::promise and mostly exists to make it
 * easier to use Future
 */
template<typename T>
class Promise {
public:
  Promise() = default;
  ~Promise() = default;
  JMG_DEFAULT_MOVEABLE(Promise);
  JMG_NON_COPYABLE(Promise);

  auto get_future() { return Future(prm_.get_future()); }

  void set_value(const T& value) {
    prm_.set_value(value);
    is_value_set_ = true;
  }

  void set_value(T&& value) {
    prm_.set_value(std::forward<T>(value));
    is_value_set_ = true;
  }

  void set_value_at_thread_exit(const T& value) {
    prm_.set_value_at_thread_exit(value);
    is_value_set_ = true;
  }

  void set_value_at_thread_exit(T&& value) {
    prm_.set_value_at_thread_exit(std::forward<T>(value));
    is_value_set_ = true;
  }

  void set_exception(std::exception_ptr ptr) {
    prm_.set_exception(ptr);
    is_value_set_ = true;
  }

  void set_exception_at_thread_exit(std::exception_ptr ptr) {
    prm_.set_exception_at_thread_exit(ptr);
    is_value_set_ = true;
  }

private:
  bool is_value_set_ = false;
  std::promise<T> prm_;
};

/**
 * specialization for void
 */
template<>
class Promise<void> {
public:
  Promise() = default;
  ~Promise() = default;
  JMG_DEFAULT_MOVEABLE(Promise);
  JMG_NON_COPYABLE(Promise);

  auto get_future() { return Future(prm_.get_future()); }

  void set_value() {
    prm_.set_value();
    is_value_set_ = true;
  }

  void set_value_at_thread_exit() {
    prm_.set_value_at_thread_exit();
    is_value_set_ = true;
  }

  void set_exception(std::exception_ptr ptr) {
    prm_.set_exception(ptr);
    is_value_set_ = true;
  }

  void set_exception_at_thread_exit(std::exception_ptr ptr) {
    prm_.set_exception_at_thread_exit(ptr);
    is_value_set_ = true;
  }

private:
  bool is_value_set_ = false;
  std::promise<void> prm_;
};

/**
 * construct a promise/future pair that communicate a value of a
 * specific type
 */
template<typename T>
auto makeCommunicator() {
  auto sndr = Promise<T>();
  auto rcvr = sndr.get_future();
  return make_tuple(std::move(sndr), std::move(rcvr));
}

} // namespace jmg
