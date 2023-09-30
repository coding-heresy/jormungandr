#pragma once

#include <stdexcept>
#include <sstream>

/**
 * macro double expansion trick to produce a string from a non-string
 * token that is already a macro, (e.g. __LINE__).
 */
#define JMG_STRINGIFY_(x) #x
#define JMG_STRINGIFY(x) JRL_STRINGIFY_(x)

#define JMG_ERR_MSG_LOCATION "at " __FILE__ "(" JMG_STRINGIFY(__LINE__) "), "

/**
 * produce an error message that has file name and line number
 * embedded in it.
 */
#define JMG_ERR_MSG_HELPER(msg) JMG_ERR_MSG_LOCATION msg

/**
 * throw an instance of std::system_error which embeds the file name,
 * line number and a user defined string.
 */
#define JMG_THROW_SYSTEM_ERROR(msg)					\
  do {									\
    std::ostringstream ss;						\
    const std::string location(ERR_MSG_LOCATION);			\
    ss << location << msg;						\
    throw std::system_error(errno, std::system_category(), ss.str());	\
  } while (0)

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
#define JMG_ENFORCE_ANY(predicate, ExceptionType, msg)	\
  do {							\
    if (!(predicate)) {					\
      JMG_THROW_EXCEPTION(ExceptionType, msg);		\
    }							\
  } while (0)

/**
 * throw an exception of type std::runtime_error constructed with the
 * argument error message if a predicate fails
 */
#define JMG_ENFORCE(predicate, msg)				\
  do {								\
    JMG_ENFORCE_ANY(predicate, std::runtime_error, msg);	\
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
    if (errVal == (func)) {				\
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

////////////////////////////////////////////////////////////////////////////////
// Helper macros for commonly used declarations
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
