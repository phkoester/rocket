/*
 * message.cc
 */

#include "rocket/str/message/message.h"

#include "rocket/format/std.h"
#include "rocket/nio/nio.h"

using namespace std;

namespace rocket::str::message {

// Functions ------------------------------------------------------------------------------------------------

string
cannotScanAs(string_view input, const type_info& type) {
  return fmt::format("Cannot scan {:?} as `{}`", input, type);
}

string
overflow(const type_info& type, string_view msg) {
  if (msg.empty()) {
    return fmt::format("`{}` overflow", type);
  } else {
    return fmt::format("`{}` overflow: {}", type, msg);
  }
}

string
withSourceLocation(string_view msg, const optional<source_location>& sl) {
  nio::StringSink out;
  if (sl) {
    out.print("{}:{}: ", sl->file_name(), sl->line());
  }
  out.write(msg);
  return out.str();
}

} // namespace rocket::str::message

// EOF
