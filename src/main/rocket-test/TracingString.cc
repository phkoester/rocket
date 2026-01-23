/*
 * TracingString.cc
 */

#include "TracingString.h"

#include "rocket/format/format.h"

using namespace std;

namespace rocket::gtest {

u64 TracingString::ID_COUNTER = 0;
u64 TracingString::NUM_INSTANCES = 0;

void TracingString::trace(std::string_view what) const {
  string msg = fmt::format("{}.{}: {}\n", id_, what, val_);
  trace_->append(msg);
}

} // namespace rocket::gtest

// EOF
