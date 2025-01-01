/*
 * assert.cc
 */

#include "codec-std-decl.h"
#include "codec-std.h"

#include "assert.h"

#include "Process.h"
#include "S.h"
#include "except.h"

using namespace rocket;
using namespace std;

// Internal -------------------------------------------------------------------------------------------------

namespace rocket::assert::internal {

void
onAssertFailed(
    const char* expr,
    const optional<string>& msg,
    const source_location& sourceLoc) {
  ostringstream buf;
  buf << sourceLoc.file_name() << ':' << sourceLoc.line() << ": Assertion " << (S << string_view(expr)) << " failed";
  if (msg)
    buf << ": " << *msg;

  process.error(cerr, buf.str(), EXIT_SUCCESS);
  terminate();
}

void
onCheckFailed(
    const char* name,
    const char* expr,
    const optional<string>& msg,
    const source_location& sourceLoc) {
  ostringstream buf;
  buf << "Check " << (S << string_view(expr)) << " failed";
  if (msg)
    buf << ": " << *msg;

  throw except::InvalidArgument(name, buf.str(), sourceLoc);
}

void
onExpectFailed(
    const char* expr,
    const optional<string>& msg,
    const source_location& sourceLoc) {
  ostringstream buf;
  buf << "Expectation " << (S << string_view(expr)) << " failed";
  if (msg)
    buf << ": " << *msg;

  throw except::InvalidState(buf.str(), sourceLoc);
}

} // namespace rocket::assert::internal

// EOF
