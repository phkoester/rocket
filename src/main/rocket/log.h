/**
 * @file log.h
 *
 * A logging API.
 *
 * This file may be included several times. If `NDEBUG` is not defined, the logging macros are active.
 * Otherwise, they expand to a call of #rocket::nop.
 */

// No `#pragma once` here!

#ifndef ROCKET_LOG_H
#define ROCKET_LOG_H

#ifdef NDEBUG
#include "base.h" // `rocket::nop()`
#endif
#include "cl.h"
#include "enum-decl.h"
#include "format.h"
#include "macro.h"

#include <boost/preprocessor/seq/cat.hpp>

#include <vector>

namespace rocket::log {

// `LogLevel` -----------------------------------------------------------------------------------------------

/**
 * A log-level enum, sorted from lowest to highest level.
 */
enum class LogLevel {
  none = 0, ///< Log level `none`.
  error = 1, ///< Log level `error`.
  warn = 2, ///< Log level `warn`.
  info = 3, ///< Log level `info`.
  debug = 4, ///< Log level `debug`.
  trace = 5 ///< Log level `trace`.
};

ROCKET_ENUM_DECLARE_LOCAL(LogLevel);

} // namespace rocket::log

ROCKET_ENUM_DECLARE_GLOBAL(rocket::log::LogLevel);

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

#if 0 // XXX Was machen wir damit?
void log(LogLevel level, const std::exception& ex);

void log(LogLevel level, std::exception_ptr ptr);
#endif

void logMessage(LogLevel level, std::string_view msg);

template<typename... T>
void log(LogLevel level, fmt::format_string<T...> fmt, T&&... args) {
  logMessage(level, fmt::format(fmt, std::forward<T>(args)...));
}

const std::vector<cl::Option>& opts();

} // namespace internal

} // namespace rocket::log

#endif // ROCKET_LOG_H

// Macros ---------------------------------------------------------------------------------------------------

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
 * Logs a message, using log level #rocket::log::error.
 *
 * @param fmt the format string
 * @param ... the arguments to the format string
 */
#define ROCKET_LOG_ERROR(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::error) { \
      ::rocket::log::internal::log( \
          ::rocket::log::LogLevel::error, \
          fmt \
          ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Logs a message, using log level #rocket::log::warn.
 *
 * @param fmt the format string
 * @param ... the arguments to the format string
 */
#define ROCKET_LOG_WARN(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::warn) { \
      ::rocket::log::internal::log( \
          ::rocket::log::LogLevel::warn, \
          fmt \
          ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Logs a message, using log level #rocket::log::info.
 *
 * @param fmt the format string
 * @param ... the arguments to the format string
 */
#define ROCKET_LOG_INFO(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::info) { \
      ::rocket::log::internal::log( \
          ::rocket::log::LogLevel::info, \
          fmt \
          ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Logs a message, using log level #rocket::log::debug.
 *
 * @param fmt the format string
 * @param ... the arguments to the format string
 */
#define ROCKET_LOG_DEBUG(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::debug) { \
      ::rocket::log::internal::log( \
          ::rocket::log::LogLevel::debug, \
          fmt \
          ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
    }

/**
 * Logs a message, using log level #rocket::log::trace.
 *
 * @param fmt the format string
 * @param ... the arguments to the format string
 */
#define ROCKET_LOG_TRACE(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::trace) { \
      ::rocket::log::internal::log( \
          ::rocket::log::LogLevel::trace, \
          fmt \
          ROCKET_COMMA_IF_VA_ARGS(__VA_ARGS__)); \
    }

#endif // NDEBUG

// EOF
