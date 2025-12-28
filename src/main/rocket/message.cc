/*
 * message.cc
 */

#include "message.h"

#include "nio.h"

using namespace std;

namespace rocket::message {

// Functions ------------------------------------------------------------------------------------------------

string
cannotParseAs(string_view input, const Type& type) {
  return fmt::format("Cannot parse {:?} as `{}`", input, type);
}

string
exceptionBase(string_view msg, const optional<source_location>& sl) {
  nio::StringSink sink;
  if (sl) {
    sink.print("{}:{}: ", sl->file_name(), sl->line());
  }
  sink.write(msg);
  return sink.str();
}

string
iteratorAt(const Type& type, size_t pos, string_view msg) {
  return fmt::format("`{}` at position {} {}", type, pos, msg);
}

string
overflow(const Type& type) {
  return fmt::format("`{}` overflow", type);
}

} // namespace rocket::message

// EOF
