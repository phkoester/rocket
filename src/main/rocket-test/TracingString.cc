/*
 * TracingString.cc
 */

#include "TracingString.h"

#include <fmt/format.h>

using namespace std;

namespace rocket::test {

u64 TracingString::ID_COUNTER = 0;
u64 TracingString::NUM_INSTANCES = 0;

void TracingString::trace(std::string_view what) const {
  const string msg = fmt::format("{}.{}: {}\n", id_, what, val_);
  trace_->append(msg);
}

} // namespace rocket::test

// EOF
