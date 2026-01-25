/**
 * @file log.h
 *
 * A logging API.
 *
 * @ThreadSafe
 */

#pragma once

#include "rocket/enum.h"
#include "rocket/cl/cl.h"

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

} // namespace rocket::log

/// @enum_declare{#rocket::log::LogLevel}
ROCKET_ENUM_DECLARE(rocket::log, LogLevel, LogLevel);

namespace rocket::log {

// Internal -------------------------------------------------------------------------------------------------

/// @cond undocumented

#define ROCKET_LOG_ID__(id) BOOST_PP_SEQ_CAT((rocketLog)(id)(__))

/// @endcond

namespace internal {

void log(LogLevel level, std::string_view msg);

void logBegin(LogLevel* logId, const char* function, const char* prettyFunction, const char* file, i32 line);

LogLevel logDefine(LogLevel* logId, std::string_view id);

void logEnd() noexcept;

void logInit();

template<typename... T>
void logMessage(LogLevel level, fmt::format_string<T...> fmt, T&&... args) {
  log(level, fmt::format(fmt, std::forward<T>(args)...));
}

const std::vector<cl::Option>& logOptions();

struct Log {
  const LogLevel level_;

  inline Log(LogLevel* logId, const char* function, const char* prettyFunction, const char* file, i32 line) :
      level_(*logId) {
    logBegin(logId, function, prettyFunction, file, line);
  }

  inline ~Log() noexcept { logEnd(); }
};

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * Sets the log format.
 *
 * @ThreadSafe
 *
 * @param val the log format
 */
void setLogFmt(std::string_view val);

/**
 * Sets the log level for the log ID @p id to @p val.
 *
 * If @p id is `"all"`, the log level is set for all log IDs.
 *
 * @ThreadSafe
 *
 * @param id the log ID, or `"all"`
 * @param val the log level
 */
void setLogLevel(std::string_view id, std::string_view val);

/**
 * Set the log output.
 *
 * @ThreadSafe
 *
 * @param val `"-"`, `"stdout"`, or `"stderr"`, or a pattern.
 */
void setLogOut(std::string_view val);

} // namespace rocket::log

// Macros ---------------------------------------------------------------------------------------------------

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
 */
#define ROCKET_LOG(id) \
    ::std::unique_ptr<::rocket::log::internal::Log> rocketLog__; \
    if (::rocket::log::internal::ROCKET_LOG_ID__(id) > ::rocket::log::LogLevel::none) { \
      rocketLog__ = ::std::make_unique<::rocket::log::internal::Log>( \
          &::rocket::log::internal::ROCKET_LOG_ID__(id), \
          __FUNCTION__, \
          __PRETTY_FUNCTION__, \
          __FILE__, \
          __LINE__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, enables logging in a function, using log ID @p id.
 *
 * @param id the log ID
 */
#define ROCKET_DEBUG_LOG(id)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, enables logging in a function, using log ID @p id.
 *
 * @param id the log ID
 */
#define ROCKET_DEBUG_LOG(id) ROCKET_LOG(id)
#endif // NDEBUG

/**
 * Logs a message, using log level #rocket::log::error.
 *
 * Usage: `ROCKET_LOG_ERROR(fmt, [args]...])`
 */
#define ROCKET_LOG_ERROR(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::error) { \
      ::rocket::log::internal::logMessage( \
          ::rocket::log::LogLevel::error, \
          fmt \
          __VA_OPT__(,) __VA_ARGS__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::error.
 *
 * Usage: `ROCKET_DEBUG_LOG_ERROR(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_ERROR(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::error.
 *
 * Usage: `ROCKET_LOG_ERROR(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_ERROR(fmt, ...) ROCKET_LOG_ERROR(fmt, __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

/**
 * Logs a message, using log level #rocket::log::warn.
 *
 * Usage: `ROCKET_LOG_WARN(fmt, [args]...])`
 */
#define ROCKET_LOG_WARN(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::warn) { \
      ::rocket::log::internal::logMessag( \
          ::rocket::log::LogLevel::warn, \
          fmt \
          __VA_OPT__(,) __VA_ARGS__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::warn.
 *
 * Usage: `ROCKET_DEBUG_LOG_WARN(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_WARN(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::warn.
 *
 * Usage: `ROCKET_LOG_ERROR(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_WARN(fmt, ...) ROCKET_LOG_WARN(fmt, __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

/**
 * Logs a message, using log level #rocket::log::info.
 *
 * Usage: `ROCKET_LOG_INFO(fmt, [args]...])`
 */
#define ROCKET_LOG_INFO(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::info) { \
      ::rocket::log::internal::logMessage( \
          ::rocket::log::LogLevel::info, \
          fmt \
          __VA_OPT__(,) __VA_ARGS__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::info.
 *
 * Usage: `ROCKET_DEBUG_LOG_INFO(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_INFO(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::info.
 *
 * Usage: `ROCKET_LOG_INFO(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_INFO(fmt, ...) ROCKET_LOG_INFO(fmt, __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

/**
 * Logs a message, using log level #rocket::log::debug.
 *
 * Usage: `ROCKET_LOG_DEBUG(fmt, [args]...])`
 */
#define ROCKET_LOG_DEBUG(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::debug) { \
      ::rocket::log::internal::logMessage( \
          ::rocket::log::LogLevel::debug, \
          fmt \
          __VA_OPT__(,) __VA_ARGS__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::debug.
 *
 * Usage: `ROCKET_DEBUG_LOG_DEBUG(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_DEBUG(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::debug.
 *
 * Usage: `ROCKET_LOG_DEBUG(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_DEBUG(fmt, ...) ROCKET_LOG_DEBUG(fmt, __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

/**
 * Logs a message, using log level #rocket::log::trace.
 *
 * Usage: `ROCKET_LOG_TRACE(fmt, [args]...])`
 */
#define ROCKET_LOG_TRACE(fmt, ...) \
    if (rocketLog__ && rocketLog__->level_ >= ::rocket::log::LogLevel::trace) { \
      ::rocket::log::internal::logMessage( \
          ::rocket::log::LogLevel::trace, \
          fmt \
          __VA_OPT__(,) __VA_ARGS__); \
    }

#ifdef NDEBUG
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::trace.
 *
 * Usage: `ROCKET_DEBUG_LOG_TRACE(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_TRACE(fmt, ...)
#else
/**
 * Only in debug code, where `NDEBUG` is not defined, logs a message, using log level #rocket::log::trace.
 *
 * Usage: `ROCKET_LOG_TRACE(fmt, [args]...])`
 */
#define ROCKET_DEBUG_LOG_TRACE(fmt, ...) ROCKET_LOG_TRACE(fmt, __VA_OPT__(,) __VA_ARGS__)
#endif // NDEBUG

// EOF
