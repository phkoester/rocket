/**
 * @file io-log.h
 *
 * Internal I/O logging. Not a public header file.
 */

#pragma once

/* Macros ---------------------------------------------------------------------------------------------------

Because #rocket::log utilizes #rocket::nio and #rocket::io, we can't use it to log #rocket::nio and
#rocket::io itself. So we make up a quick and dirty logging facility here.

---------------------------------------------------------------------------------------------------------- */

#ifdef ROCKET_IO_LOG
#define IO_LOG(args) cout << "# " << ROCKET_SRC_FILE << ':' << __LINE__ << ' ' << __FUNCTION__ << ": " << args << endl;
#else
#define IO_LOG(args)
#endif

// EOF
