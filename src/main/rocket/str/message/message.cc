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
  }
  return fmt::format("`{}` overflow: {}", type, msg);
}

string
underflow(const type_info& type, string_view msg) {
  if (msg.empty()) {
    return fmt::format("`{}` underflow", type);
  }
  return fmt::format("`{}` underflow: {}", type, msg);
}

string
withSourceLocation(string_view msg, const optional<source_location>& sl) {
  if (sl) {
    return fmt::format("{}:{}: {}", sl->file_name(), sl->line(), msg);
  }
  return string(msg);
}

} // namespace rocket::str::message

// EOF
