/*
 * IoError.cc
 */

#include "rocket/io/IoError.h"

#include "rocket/str/message/message.h"

using namespace std;

namespace rocket::io {

// `IoError` ------------------------------------------------------------------------------------------------

IoError::IoError(
  string_view msg,
  const optional<source_location>& sl,
  const optional<stacktrace>& st) :
  Exception(msg, sl, st),
  Base(str::message::withSourceLocation(Exception::message(), Exception::sourceLocation())) {}

} // namespace rocket::io

// EOF
