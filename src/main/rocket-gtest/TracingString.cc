/*
 * TracingString.cc
 */

#include "TracingString.h"

#include "rocket/format/format.h"

using namespace std;

namespace rocket::gtest {

size_t TracingString::ID_COUNTER = 0;
size_t TracingString::NUM_INSTANCES = 0;

void TracingString::trace(std::string_view what) const {
  string msg = fmt::format("{}.{}: {}\n", id_, what, v_);
  trace_->append(msg);
}

} // namespace rocket::gtest

// EOF
