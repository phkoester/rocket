/*
 * Exception.cc
 */

#include "Exception.h"

#include "rocket/assert.h"
#include "rocket/str/message/message.h"

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

void printThrown(
  nio::Sink&, size_t, const optional<Type>&, const optional<string>&, const optional<stacktrace>&);

string
getWhat(int v) {
  return fmt::format("{}", v);
}

string
getWhat(long v) {
  return fmt::format("{}", v);
}

string
getWhat(const char* v) {
  if (not v)
    return "null";
  return fmt::format("{:?}", v); // With quotation marks
}

string
getWhat(const string& v) {
  return fmt::format("{:?}", v); // With quotation marks
}

string
getWhat(string_view v) {
  return fmt::format("{:?}", v); // With quotation marks
}

string
getWhat(const exception& v) {
  const char* p = v.what();
  if (not p)
    return "<null>";
  string_view s(p);
  if (s.empty())
    return "<none>";
  return fmt::format("{}", s);
}

void
printExceptionPtr(nio::Sink& out, size_t level, const exception_ptr& ptr) {
  try {
    rethrow_exception(ptr);
  } catch (const exception& ex) {
    const Exception* p = dynamic_cast<const Exception*>(&ex);
    printThrown(out, level, Type::of(ex), getWhat(ex), p ? p->stackTrace() : nullopt);
    try {
      rethrow_if_nested(ex);
    } catch (...) {
      printExceptionPtr(out, level + 1, current_exception());
    }
  } catch (int v) {
    printThrown(out, level, Type::of(v), getWhat(v), nullopt);
  } catch (long v) {
    printThrown(out, level, Type::of(v), getWhat(v), nullopt);
  } catch (const char* v) {
    printThrown(out, level, Type::of(v), getWhat(v), nullopt);
  } catch (const string& v) {
    printThrown(out, level, Type::of(v), getWhat(v), nullopt);
  } catch (string_view v) { // cppcheck-suppress catchExceptionByValue
    printThrown(out, level, Type::of(v), getWhat(v), nullopt);
  } catch (...) {
    const type_info* info = current_exception().__cxa_exception_type();
    if (info)
      printThrown(out, level, Type(*info), nullopt, nullopt);
    else
      printThrown(out, level, nullopt, nullopt, nullopt);
  }
}

void
printThrown(
    nio::Sink& out,
    size_t level,
    const optional<Type>& type,
    const optional<string>& what,
    const optional<stacktrace>& st) {
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
  if (what) {
    msg.print(": {}", *what);
  }
  out.writeln(msg.str());

  if (st) {
    ostringstream os;
    os << *st; // This prints a '\n' at the end
    out.write(os.str());
  }
}

void
whatExceptionPtr(nio::Sink& out, size_t level, const exception_ptr& ptr) {
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
  } catch (int v) {
    out.write(getWhat(v));
  } catch (long v) {
    out.write(getWhat(v));
  } catch (const char* v) {
    out.write(getWhat(v));
  } catch (const string& v) {
    out.write(getWhat(v));
  } catch (string_view v) { // cppcheck-suppress catchExceptionByValue
    out.write(getWhat(v));
  } catch (...) {}

  if (level > 0) {
    out.write(')');
  }
}

} // namespace

namespace rocket {

// `InvalidArgument` ----------------------------------------------------------------------------------------

InvalidArgument::InvalidArgument(
    string_view name,
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Base(str::message::exceptionBase(fmt::format("Parameter `{}`: {}", name, msg), sl)),
    Exception(msg, std::move(sl), std::move(st)) {}

// `InvalidState` -------------------------------------------------------------------------------------------

InvalidState::InvalidState(
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Base(str::message::exceptionBase(msg, sl)),
    Exception(msg, std::move(sl), std::move(st)) {}

// `Overflow` -----------------------------------------------------------------------------------------------

Overflow::Overflow(
  const Type& type,
  string_view msg,
  optional<source_location>&& sl,
  optional<stacktrace>&& st) :
  Base(str::message::overflow(type, msg)),
  Exception(msg, std::move(sl), std::move(st)) {}

// Functions ------------------------------------------------------------------------------------------------

void
printException(nio::Sink& out, const exception& ex) {
  const Exception* p = dynamic_cast<const Exception*>(&ex);
  printThrown(out, 0, Type::of(ex), getWhat(ex), p ? p->stackTrace() : nullopt);

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
