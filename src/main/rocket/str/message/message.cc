/*
 * message.cc
 */

#include "rocket/str/message/message.h"

#include "rocket/format/std.h"

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
  if (sl) {
    return fmt::format("{}:{}: {}", sl->file_name(), sl->line(), msg);
  } else {
    return string(msg);
  }
}

} // namespace rocket::str::message

// EOF
