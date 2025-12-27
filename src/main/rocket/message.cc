/*
 * message.cc
 */

#include "message.h"

#include "nio.h"

using namespace std;

namespace rocket::message {

// Messages -------------------------------------------------------------------------------------------------

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
overflow(const Type& type) {
  return fmt::format("`{}` overflow", type);
}

} // namespace rocket::message

// EOF
