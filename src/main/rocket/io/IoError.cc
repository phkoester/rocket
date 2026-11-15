/*
 * IoError.cc
 */

#include "rocket/format.h"
#include "rocket/io/IoError.h"
#include "rocket/str/message/message.h"

using namespace std;

namespace rocket::io {

// `IoError` ------------------------------------------------------------------------------------------------

IoError::IoError(
  const error_code& ec,
  string_view msg,
  const optional<source_location>& sl,
  const optional<stacktrace>& st) :
  Exception(fmt::format("{}: {}", msg, ec.message()), sl, st),
  system_error(ec, str::message::withSourceLocation(Exception::message(), Exception::sourceLocation()))
{}

} // namespace rocket::io

// EOF
