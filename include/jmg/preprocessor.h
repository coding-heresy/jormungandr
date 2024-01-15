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

#include <stdexcept>
#include <sstream>

/**
 * macro double expansion trick to produce a string from a non-string
 * token that is already a macro, (e.g. __LINE__).
 */
#define JMG_STRINGIFY_(x) #x
#define JMG_STRINGIFY(x) JMG_STRINGIFY_(x)

#define JMG_ERR_MSG_LOCATION "at " __FILE__ "(" JMG_STRINGIFY(__LINE__) "), "

////////////////////////////////////////////////////////////////////////////////
// branch prediction hints
////////////////////////////////////////////////////////////////////////////////

#define JMG_UNLIKELY(pred) __builtin_expect(pred, false)

////////////////////////////////////////////////////////////////////////////////
// macro that simplifies definition of new custom exception types
////////////////////////////////////////////////////////////////////////////////

/**
 * easily define an exception type that derives from
 * std::runtime_error
 */
#define JMG_DEFINE_RUNTIME_EXCEPTION(name)	\
  class name : public std::runtime_error	\
  {						\
  public:					\
    name(const std::string &what)		\
      : std::runtime_error(what) {}		\
}

////////////////////////////////////////////////////////////////////////////////
// macros that simplify throwing exceptions in error conditions
////////////////////////////////////////////////////////////////////////////////

/**
 * produce an error message that has file name and line number
 * embedded in it.
 */
#define JMG_ERR_MSG_HELPER(msg) JMG_ERR_MSG_LOCATION msg

/**
 * throw an instance of std::system_error which embeds the file name,
 * line number and a user defined string.
 */
#define JMG_THROW_SYSTEM_ERROR_FROM_ERRNO(err_num, msg)			\
  do {									\
    std::ostringstream ss;						\
    const std::string location(JMG_ERR_MSG_LOCATION);			\
    ss << location << msg;						\
    throw std::system_error(err_num, std::system_category(), ss.str());	\
  } while (0)

/**
 * throw an instance of std::system_error which embeds the file name,
 * line number and a user defined string.
 */
#define JMG_THROW_SYSTEM_ERROR(msg)		\
  JMG_THROW_SYSTEM_ERROR_FROM_ERRNO(errno, msg)

/**
 * helper macro for simplifying the use of exceptions.
 *
 * @todo use basename of filename?
 */
#define JMG_THROW_EXCEPTION(ExceptionType, msg)				\
  do {									\
    std::ostringstream what;						\
    what << "'" << msg << "' on line " << __LINE__ << " of file " << __FILE__; \
    throw ExceptionType(what.str());					\
  } while(0)

/**
 * throw an exception of a specified type constructed with the
 * argument error message if a predicate fails
 */
#define JMG_ENFORCE_USING(exception_type, predicate, msg)	\
  do {								\
    if (JMG_UNLIKELY(!(predicate))) {				\
      JMG_THROW_EXCEPTION(exception_type, msg);			\
    }								\
  } while (0)

/**
 * throw an exception of type std::runtime_error constructed with the
 * argument error message if a predicate fails
 */
#define JMG_ENFORCE(predicate, msg)				\
  do {								\
    JMG_ENFORCE_USING(std::runtime_error, predicate, msg);	\
  } while (0)

/**
 * helper macro for wrapping the behavior of calling a POSIX-style system
 * function, checking the return code against a known error value and
 * throwing an appropriate exception if an error is detected.
 *
 * NOTE: this only works for functions which return a single error
 * value (e.g. -1 or nullptr) and set errno appropriately, which does
 * not include several classes of system functions
 * (e.g. pthread_create, which returns 0 on success and an error code
 * on error, or gethostbyname_r, which returns nullptr on error but
 * uses h_errno to report the cause)
 */
#define JMG_CALL_SYSTEM_FUNC(func, errVal, errMsg)	\
  do {							\
    if (JMG_UNLIKELY(errVal == (func))) {		\
      JMG_THROW_SYSTEM_ERROR(errMsg);			\
    }							\
  } while (0)

/**
 * call a POSIX-style system function that returns -1 on error (the
 * most common behavior) and throw an exception of type
 * std::system_error if it fails
 */
#define JMG_SYSTEM(func, errMsg)		\
  do {						\
    JMG_CALL_SYSTEM_FUNC(func, -1, errMsg);	\
  } while (0)

/**
 * call a POSIX-style system function that returns 0 on success and
 * -errno on failure, and throw an exception of type std::system_error
 * if it fails
 */
#define JMG_SYSTEM_ERRNO_RETURN(func, errMsg)				\
  do {									\
    const auto rc = (func);						\
    if (JMG_UNLIKELY(rc != 0)) {					\
      JMG_THROW_SYSTEM_ERROR_FROM_ERRNO(-rc, errMsg);			\
    }									\
  } while (0)

////////////////////////////////////////////////////////////////////////////////
// helper macros for commonly used declarations
////////////////////////////////////////////////////////////////////////////////

#define JMG_DEFAULT_COPYABLE(Class)		\
  Class(const Class &src) = default;		\
  Class& operator=(const Class &rhs) = default

#define JMG_NON_COPYABLE(Class)			\
  Class(const Class &src) = delete;		\
  Class& operator=(const Class &rhs) = delete

#define JMG_DEFAULT_MOVEABLE(Class)		\
  Class(Class &&src) = default;			\
  Class& operator=(Class &&rhs) = default

#define JMG_NON_MOVEABLE(Class)			\
  Class(Class &&src) = delete;			\
  Class& operator=(Class &&rhs) = delete
