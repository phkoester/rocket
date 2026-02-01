/*
 * chrono.cc
 */

#include "chrono.h"

using namespace std;
using namespace std::chrono;

namespace rocket::chrono::internal {

// Internal -------------------------------------------------------------------------------------------------

ROCKET_PUBLIC recursive_mutex clockMutex;

ROCKET_PUBLIC milliseconds clockOffset = 0ms;

} // namespace rocket::chrono::internal

// EOF
