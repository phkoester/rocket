/*
 * chrono.cc
 */

#include "chrono.h"

#include "rocket/macro.h"

#include <mutex>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

// Local variables ------------------------------------------------------------------------------------------

namespace {

recursive_mutex clockMutex;

hours clockOffset = 0h;

} // namespace

namespace rocket::chrono {

namespace internal {

// Internal -------------------------------------------------------------------------------------------------

void
setClockOffset(hours offset) {
  ROCKET_MUTEX_LOCK(clockMutex);
  clockOffset = offset;
}

} // namespace internal

// Functions -----------------------------------------------------------------------------------------------

SystemClockTimePoint
systemClockNow() {
  ROCKET_MUTEX_LOCK(clockMutex);
  return system_clock::now() + clockOffset;
}

} // namespace rocket::chrono

// EOF
