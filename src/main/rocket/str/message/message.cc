/*
 * message.cc
 */

#include "rocket/str/message/message.h"

#include "rocket/nio/nio.h"

using namespace std;

namespace rocket::str::message {

// Functions ------------------------------------------------------------------------------------------------

string
cannotParseAs(string_view input, const Type& type) {
  return fmt::format("Cannot parse {:?} as `{}`", input, type);
}

string
exceptionBase(string_view msg, const optional<source_location>& sl) {
  nio::StringSink out;
  if (sl) {
    out.print("{}:{}: ", sl->file_name(), sl->line());
  }
  out.write(msg);
  return out.str();
}

string
iteratorAt(const Type& type, size_t pos, string_view msg) {
  return fmt::format("`{}` at position {} {}", type, pos, msg);
}

string
overflow(const Type& type, string_view msg) {
  if (msg.empty()) {
    return fmt::format("`{}` overflow", type);
  } else {
    return fmt::format("`{}` overflow: {}", type, msg);
  }
}

} // namespace rocket::str::message

// EOF
