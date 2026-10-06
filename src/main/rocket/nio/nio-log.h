/**
 * @file nio-log.h
 *
 * Logging for `rocket::nio`. Not a public header file.
 */

#pragma once

/* Macros ---------------------------------------------------------------------------------------------------

Because #rocket::log utilizes #rocket::nio, which in turn uses #rocket::io, we can't use the logging API to
log #rocket::nio itself. So we make up a quick and dirty logging facility here.

---------------------------------------------------------------------------------------------------------- */

/**
 * Logging macro.
 *
 * Usage: `NIO_LOG("a=" << ", b=" << b);`
 */
#ifdef ROCKET_NO_LOG
#define NIO_LOG(args) ::std::cout << "nio# " << ROCKET_SRC_FILE << ':' << \
  __LINE__ << ' ' << __FUNCTION__ << ": " << args << ::std::endl;
#else
#define NIO_LOG(args)
#endif

// EOF
