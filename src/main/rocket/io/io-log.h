/**
 * @file io-log.h
 *
 * Logging for `rocket::io`. Not a public header file.
 */

#pragma once

/* Macros ---------------------------------------------------------------------------------------------------

Because #rocket::log utilizes #rocket::nio, which in turn uses #rocket::io, we can't use the logging API to
log #rocket::io itself. So we make up a quick and dirty logging facility here.

---------------------------------------------------------------------------------------------------------- */

/**
 * Logging macro.
 *
 * Usage: `IO_LOG("a=" << ", b=" << b);`
 */
#ifdef ROCKET_IO_LOG
#define IO_LOG(args) ::std::cout << "io# " << ROCKET_SRC_FILE << ':' << \
  __LINE__ << ' ' << __FUNCTION__ << ": " << args << '\n'
#else
#define IO_LOG(args)
#endif

// EOF
