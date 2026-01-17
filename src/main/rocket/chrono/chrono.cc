/*
 * chrono.cc
 */

#include "chrono.h"

using namespace std;
using namespace std::chrono;

namespace rocket::chrono::internal {

// Internal -------------------------------------------------------------------------------------------------

recursive_mutex clockMutex;

milliseconds clockOffset = 0ms;

} // namespace rocket::chrono::internal

// EOF
