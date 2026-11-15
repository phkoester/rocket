/*
 * chrono.cc
 */

#include "chrono.h"

using namespace std::chrono;

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

std::recursive_mutex&
clockMutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

milliseconds&
clockOffset() {
  static milliseconds offset = 0ms;
  return offset;
}

} // namespace internal

} // namespace rocket::chrono

// EOF
