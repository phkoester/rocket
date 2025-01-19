/**
 * @file log.h
 *
 * A logging API.
 *
 * This file may be included several times. If @c NDEBUG is not defined, the logging macros are active.
 * Otherwise, they expand to a call of #rocket::nop().
 */

// No '#pragma once' here!

#ifndef ROCKET_LOG_H
#define ROCKET_LOG_H

#include "basic.h" // 'rocket::nop()'
#include "cl.h"
#include "enum-decl.h"

#include <boost/preprocessor/seq/cat.hpp>

#include <exception>
#include <vector>

namespace rocket::log {

// 'LogLevel' -----------------------------------------------------------------------------------------------

/**
 * A log-level enum, sorted from lowest to highest level.
 */
enum class LogLevel {
  none = 0, ///< Log level @c none.
  error = 1, ///< Log level @c error.
  warn = 2, ///< Log level @c warn.
  info = 3, ///< Log level @c info.
  debug = 4, ///< Log level @c debug.
  trace = 5 ///< Log level @c trace.
};

/// @enum_declare{#rocket::log::LogLevel}
ROCKET_ENUM_DECLARE(LogLevel);

} // namespace rocket::log

/// @spec_std_formatter{#rocket::log::LogLevel}
ROCKET_ENUM_DECLARE_STD_FORMATTER(rocket::log::LogLevel);

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_LOG_ID__(id) BOOST_PP_SEQ_CAT((rocketLog)(id)(__))

/// @endcond

namespace internal {

void init();

LogLevel logDefine(LogLevel* logId, std::string_view id);

void logBegin(LogLevel* logId, const char* func);

void logEnd() noexcept;

struct Log {
  inline Log(LogLevel* logId, const char* func) :
      level_(*logId) {
    logBegin(logId, func);
  }

  inline ~Log() noexcept { logEnd(); }

  const LogLevel level_;
};

void log(LogLevel level, const std::exception& ex);

void log(LogLevel level, std::exception_ptr ptr);

void log(LogLevel level, std::string_view msg);

const std::vector<cl::Option>& opts();

} // namespace internal

} // namespace rocket::log

#endif // ROCKET_LOG_H

// End of header guard --------------------------------------------------------------------------------------

#undef ROCKET_LOG_DECLARE
#undef ROCKET_LOG_DEFINE

#undef ROCKET_LOG

#undef ROCKET_LOG_ERROR
#undef ROCKET_LOG_WARN
#undef ROCKET_LOG_INFO
#undef ROCKET_LOG_DEBUG
#undef ROCKET_LOG_TRACE

#ifdef NDEBUG

#define ROCKET_LOG_DECLARE(id) ::rocket::nop()
#define ROCKET_LOG_DEFINE(id) ::rocket::nop()

#define ROCKET_LOG(id) ::rocket::nop()

#define ROCKET_LOG_ERROR(v) ::rocket::nop()
#define ROCKET_LOG_WARN(v) ::rocket::nop()
#define ROCKET_LOG_INFO(v) ::rocket::nop()
#define ROCKET_LOG_DEBUG(v) ::rocket::nop()
#define ROCKET_LOG_TRACE(v) ::rocket::nop()

#else

/**
 * Declares the log ID @p id.
 *
 * @param id the log ID
 */
#define ROCKET_LOG_DECLARE(id) \
    namespace rocket::log::internal { \
      extern LogLevel ROCKET_LOG_ID__(id); \
    }

/**
 * Defines the log ID @p id.
 *
 * @param id the log ID
 */
#define ROCKET_LOG_DEFINE(id) \
    namespace rocket::log::internal { \
      LogLevel ROCKET_LOG_ID__(id) = logDefine(&ROCKET_LOG_ID__(id), #id); \
    }

/**
 * Enables logging in a function, using log ID @p id.
 *
 * @param id the log ID
 *
 * @ThreadSafe
 */
#define ROCKET_LOG(id) \
    ::std::unique_ptr<::rocket::log::internal::Log> rocketLog__; \
    if (::rocket::log::internal::ROCKET_LOG_ID__(id) > ::rocket::log::LogLevel::none) \
      rocketLog__ = ::std::make_unique<::rocket::log::internal::Log>( \
          &::rocket::log::internal::ROCKET_LOG_ID__(id), __PRETTY_FUNCTION__)

/**
 * Logs an object, using log level #rocket::log::error.
 *
 * @param v the object to log
 */
#define ROCKET_LOG_ERROR(v) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::error) \
      ::rocket::log::internal::log(::rocket::log::LogLevel::error, v)

/**
 * Logs an object, using log level #rocket::log::warn.
 *
 * @param v the object to log
 */
#define ROCKET_LOG_WARN(v) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::warn) \
      ::rocket::log::internal::log(::rocket::log::LogLevel::warn, v)

/**
 * Logs an object, using log level #rocket::log::info.
 *
 * @param v the object to log
 */
#define ROCKET_LOG_INFO(v) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::info) \
      ::rocket::log::internal::log(::rocket::log::LogLevel::info, v)

/**
 * Logs an object, using log level #rocket::log::debug.
 *
 * @param v the object to log
 */
#define ROCKET_LOG_DEBUG(v) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::debug) \
      ::rocket::log::internal::log(::rocket::log::LogLevel::debug, v)

/**
 * Logs an object, using log level #rocket::log::trace.
 *
 * @param v the object to log
 */
#define ROCKET_LOG_TRACE(v) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::trace) \
      ::rocket::log::internal::log(::rocket::log::LogLevel::trace, v)

#endif // NDEBUG

// EOF
