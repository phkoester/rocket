/*
 * Exception.cc
 */

#include "Exception.h"

#include "rocket/assert.h"
#include "rocket/format/std.h"
#include "rocket/str/message/message.h"

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

void printThrown(
  nio::Sink&, u64, const type_info*, const optional<string>&, const optional<stacktrace>&);

string
getWhat(i32 val) {
  return fmt::format("{}", val);
}

string
getWhat(i64 val) {
  return fmt::format("{}", val);
}

string
getWhat(const char* val) {
  if (not val) {
    return "<null>";
  }
  return fmt::format("{:?}", val); // With quotation marks
}

string
getWhat(const string& val) {
  return fmt::format("{:?}", val); // With quotation marks
}

string
getWhat(string_view val) {
  return fmt::format("{:?}", val); // With quotation marks
}

string
getWhat(const exception& val) {
  const char* p = val.what();
  if (not p) {
    return "<null>";
  }
  string_view str(p);
  if (str.empty())
    return "<none>";
  return fmt::format("{}", str);
}

void
printExceptionPtr(nio::Sink& out, u64 level, const exception_ptr& ptr) {
  try {
    rethrow_exception(ptr);
  } catch (const exception& ex) {
    const Exception* p = dynamic_cast<const Exception*>(&ex);
    printThrown(out, level, &typeid(ex), getWhat(ex), p ? p->stackTrace() : nullopt);
    try {
      rethrow_if_nested(ex);
    } catch (...) {
      printExceptionPtr(out, level + 1, current_exception());
    }
  } catch (i32 val) {
    printThrown(out, level, &typeid(val), getWhat(val), nullopt);
  } catch (i64 val) {
    printThrown(out, level, &typeid(val), getWhat(val), nullopt);
  } catch (const char* val) {
    printThrown(out, level, &typeid(val), getWhat(val), nullopt);
  } catch (const string& val) {
    printThrown(out, level, &typeid(val), getWhat(val), nullopt);
  } catch (string_view val) { // cppcheck-suppress catchExceptionByValue
    printThrown(out, level, &typeid(val), getWhat(val), nullopt);
  } catch (...) {
#ifdef ROCKET_OS_WINDOWS
    printThrown(out, level, nullptr, nullopt, nullopt);
#else
    const type_info* type = current_exception().__cxa_exception_type();
    if (type) {
      printThrown(out, level, type, nullopt, nullopt);
    } else {
      printThrown(out, level, nullptr, nullopt, nullopt);
    }
#endif
  }
}

void
printThrown(
    nio::Sink& out,
    u64 level,
    const type_info* type,
    const optional<string>& what,
    const optional<stacktrace>& st) {
  // Print information about the instance

  nio::StringSink instanceOf;
  if (type) {
    instanceOf.print("instance of `{}`", *type);
  } else {
    instanceOf.write("instance of an unknown type");
  }

  nio::StringSink msg;
  if (level == 0) {
    msg.print("An {} was thrown", instanceOf.str());
  } else {
    msg.print("Caused by an {}", instanceOf.str());
  }

  // Print the message

  if (what) {
    msg.print(": {}", *what);
  }
  out.writeln(msg.str());

  // Print the stack trace

  if (st) {
    ostringstream buf;
    buf << *st; // This prints a '\n' at the end
    out.write(buf.str());
  }
}

void
whatExceptionPtr(nio::Sink& out, u64 level, const exception_ptr& ptr) {
  if (level > 0) {
    out.write(" (Because: ");
  }

  try {
    rethrow_exception(ptr);
  } catch (const exception& ex) {
    out.write(getWhat(ex));
    try {
      rethrow_if_nested(ex);
    } catch (...) {
      whatExceptionPtr(out, level + 1, current_exception());
    }
  } catch (i32 val) {
    out.write(getWhat(val));
  } catch (i64 val) {
    out.write(getWhat(val));
  } catch (const char* val) {
    out.write(getWhat(val));
  } catch (const string& val) {
    out.write(getWhat(val));
  } catch (string_view val) { // cppcheck-suppress catchExceptionByValue
    out.write(getWhat(val));
  } catch (...) {}

  if (level > 0) {
    out.write(')');
  }
}

} // namespace

namespace rocket {

// #InvalidArgument -----------------------------------------------------------------------------------------

InvalidArgument::InvalidArgument(
    string_view name,
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Exception(fmt::format("Parameter `{}`: {}", name, msg), std::move(sl), std::move(st)),
    Base(str::message::withSourceLocation(Exception::message(), Exception::sourceLocation())) {}

// #InvalidState --------------------------------------------------------------------------------------------

InvalidState::InvalidState(
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Exception(msg, std::move(sl), std::move(st)),
    Base(str::message::withSourceLocation(Exception::message(), Exception::sourceLocation())) {}

// #Overflow ------------------------------------------------------------------------------------------------

Overflow::Overflow(
  const type_info& type,
  string_view msg,
  optional<source_location>&& sl,
  optional<stacktrace>&& st) :
  Exception(str::message::overflow(type, msg), std::move(sl), std::move(st)),
  Base(str::message::withSourceLocation(Exception::message(), Exception::sourceLocation())) {}

// #Underflow ------------------------------------------------------------------------------------------------

Underflow::Underflow(
  const type_info& type,
  string_view msg,
  optional<source_location>&& sl,
  optional<stacktrace>&& st) :
  Exception(str::message::underflow(type, msg), std::move(sl), std::move(st)),
  Base(str::message::withSourceLocation(Exception::message(), Exception::sourceLocation())) {}

// Functions ------------------------------------------------------------------------------------------------

void
printException(nio::Sink& out, const exception& ex) {
  const Exception* p = dynamic_cast<const Exception*>(&ex);
  printThrown(out, 0, &typeid(ex), getWhat(ex), p ? p->stackTrace() : nullopt);

  try {
     rethrow_if_nested(ex);
  } catch (...) {
    printExceptionPtr(out, 1, current_exception());
  }
}

void
printException(nio::Sink& out, exception_ptr ptr) {
  ROCKET_CHECK(ptr, static_cast<bool>(ptr));
  printExceptionPtr(out, 0, ptr);
}

string
what(const exception& ex) {
  nio::StringSink out;
  out.write(getWhat(ex));
  try {
    rethrow_if_nested(ex);
  } catch (...) {
    whatExceptionPtr(out, 1, current_exception());
  }
  return out.str();
}

string
what(exception_ptr ptr) {
  ROCKET_CHECK(ptr, static_cast<bool>(ptr));
  nio::StringSink out;
  whatExceptionPtr(out, 0, ptr);
  return out.str();
}

} // namespace rocket::except

// EOF
