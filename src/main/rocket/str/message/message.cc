/*
 * message.cc
 */

#include "rocket/TypeId.h"
#include "rocket/format.h"
#include "rocket/str/message/message.h"

using namespace std;

namespace rocket::str::message {

// Functions ------------------------------------------------------------------------------------------------

string
cannotScanAs(string_view input, const type_info& type) {
  return fmt::format("Cannot scan {:?} as {}", input, TypeId(type));
}

string
overflow(const type_info& type, string_view msg) {
  if (msg.empty()) {
    return fmt::format("{} overflow", TypeId(type));
  }
  return fmt::format("{} overflow: {}", TypeId(type), msg);
}

string
underflow(const type_info& type, string_view msg) {
  if (msg.empty()) {
    return fmt::format("{} underflow", TypeId(type));
  }
  return fmt::format("{} underflow: {}", TypeId(type), msg);
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
