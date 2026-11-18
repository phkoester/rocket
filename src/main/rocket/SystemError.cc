/*
 * SystemError.cc
 */

#include "rocket/SystemError.h"
#include "rocket/str/message/message.h"

#include <fmt/std.h>

using namespace std;

namespace rocket {

// `SystemError` --------------------------------------------------------------------------------------------

SystemError::SystemError(
  const error_code& ec,
  string_view msg,
  const optional<source_location>& sl,
  const optional<stacktrace>& st) :
  Exception(fmt::format("{}: {:s}", msg, ec), sl, st),
  system_error(ec, str::message::withSourceLocation(Exception::message(), Exception::sourceLocation()))
{}

} // namespace rocket

// EOF
