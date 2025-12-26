/*
 * except.cc
 */

#include "codec-rocket-decl.h"
#include "codec-std-decl.h"
#include "codec-rocket.h"
#include "codec-std.h"

#include "except.h"

#include "S.h"
#include "assert.h"

using namespace rocket;
using namespace rocket::except;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

void printThrown(
  nio::Sink&, size_t, const optional<Type>&, const optional<string>&, const optional<stacktrace>&);

string
getWhat(int v) {
  return S << v;
}

string
getWhat(long v) {
  return S << v;
}

string
getWhat(const char* v) {
  if (not v)
    return "null";
  string_view s(v);
  return S << s; // With quotation marks
}

string
getWhat(const string& v) {
  return S << v; // With quotation marks
}

string
getWhat(string_view v) {
  return S << v; // With quotation marks
}

string
getWhat(const exception& v) {
  const char* p = v.what();
  if (not p)
    return "null";
  string_view s(p);
  if (s.empty())
    return "No message";
  return string(s); // Without quotation marks
}

void
printExceptionPtr(nio::Sink& sink, size_t level, const exception_ptr& ptr) {
  try {
    rethrow_exception(ptr);
  } catch (const exception& ex) {
    const Exception* p = dynamic_cast<const Exception*>(&ex);
    printThrown(sink, level, Type::of(ex), getWhat(ex), p ? p->stackTrace() : nullopt);
    try {
      rethrow_if_nested(ex);
    } catch (...) {
      printExceptionPtr(sink, level + 1, current_exception());
    }
  } catch (int v) {
    printThrown(sink, level, Type::of(v), getWhat(v), nullopt);
  } catch (long v) {
    printThrown(sink, level, Type::of(v), getWhat(v), nullopt);
  } catch (const char* v) {
    printThrown(sink, level, Type::of(v), getWhat(v), nullopt);
  } catch (const string& v) {
    printThrown(sink, level, Type::of(v), getWhat(v), nullopt);
  } catch (string_view v) { // cppcheck-suppress catchExceptionByValue
    printThrown(sink, level, Type::of(v), getWhat(v), nullopt);
  } catch (...) {
    const type_info* info = current_exception().__cxa_exception_type();
    if (info)
      printThrown(sink, level, Type(*info), nullopt, nullopt);
    else
      printThrown(sink, level, nullopt, nullopt, nullopt);
  }
}

void
printThrown(
    nio::Sink& sink,
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
  sink.writeln(msg.str());

  if (st) {
    ostringstream os;
    os << *st; // This prints a '\n' at the end
    sink.write(os.str());
  }
}

void
whatExceptionPtr(ostream& os, size_t level, const exception_ptr& ptr) {
  if (level > 0)
    os << " (Because: ";

  try {
    rethrow_exception(ptr);
  } catch (const exception& ex) {
    os << getWhat(ex);
    try {
      rethrow_if_nested(ex);
    } catch (...) {
      whatExceptionPtr(os, level + 1, current_exception());
    }
  } catch (int v) {
    os << getWhat(v);
  } catch (long v) {
    os << getWhat(v);
  } catch (const char* v) {
    os << getWhat(v);
  } catch (const string& v) {
    os << getWhat(v);
  } catch (string_view v) { // cppcheck-suppress catchExceptionByValue
    os << getWhat(v);
  } catch (...) {}

  if (level > 0)
    os << ')';
}

} // namespace

namespace rocket::except {

// Messages -------------------------------------------------------------------------------------------------

namespace message {

string
baseMessage(string_view msg, const optional<source_location>& sl) {
  ostringstream os;
  if (sl)
    os << sl->file_name() << ':' << sl->line() << ": ";
  os << msg;
  return os.str();
}

string
cannotParseAs(string_view input, const Type& type) {
  return S << "Cannot parse " << input << " as " << type;
}

string
overflow(const Type& type) {
  return S << type << " overflow";
}

} // namespace message

// `InvalidArgument` ----------------------------------------------------------------------------------------

InvalidArgument::InvalidArgument(
    string_view name,
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Base(message::baseMessage(fmt::format("Parameter `{}`: {}", name, msg), sl)),
    Exception(msg, std::move(sl), std::move(st)) {}

// `InvalidState` -------------------------------------------------------------------------------------------

InvalidState::InvalidState(
    string_view msg,
    optional<source_location>&& sl,
    optional<stacktrace>&& st) :
    Base(message::baseMessage(msg, sl)),
    Exception(msg, std::move(sl), std::move(st)) {}

// Functions ------------------------------------------------------------------------------------------------

void
printException(nio::Sink& sink, const exception& ex) {
  const Exception* p = dynamic_cast<const Exception*>(&ex);
  printThrown(sink, 0, Type::of(ex), getWhat(ex), p ? p->stackTrace() : nullopt);

  try {
     rethrow_if_nested(ex);
  } catch (...) {
    printExceptionPtr(sink, 1, current_exception());
  }
}

void
printException(nio::Sink& sink, exception_ptr ptr) {
  ROCKET_CHECK(ptr, static_cast<bool>(ptr));
  printExceptionPtr(sink, 0, ptr);
}

string
what(const exception& ex) {
  ostringstream os;
  os << getWhat(ex);
  try {
    rethrow_if_nested(ex);
  } catch (...) {
    whatExceptionPtr(os, 1, current_exception());
  }
  return os.str();
}

string
what(exception_ptr ptr) {
  ROCKET_CHECK(ptr, static_cast<bool>(ptr));
  ostringstream os;
  whatExceptionPtr(os, 0, ptr);
  return os.str();
}

} // namespace rocket::except

// EOF
