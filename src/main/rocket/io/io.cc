/*
 * io.cc
 */

#include "io.h"

#include "rocket/assert.h"

#include <type_traits>

using namespace std;
namespace rocket::io {

// Functions ------------------------------------------------------------------------------------------------

std::ios::pos_type
tellg(std::istream& is) noexcept {
  const auto state = is.rdstate();

  // Clear all bits
  is.clear();
  // This is expected to never throw, otherwise this implementation is flawed
  auto ret = is.tellg();
  static_assert(is_same_v<decltype(ret), std::ios::pos_type>);
  ROCKET_ASSERT(ret >= 0);

  // Restore the state
  if ((is.exceptions() & state) == 0) {
    // Restore the state without exception
    is.clear(state);
  } else {
    // Restore the state with exception
    try {
      is.clear(state);
    } catch (const std::ios::failure&) {
      // Nothing to do, we want to catch this silently
    } catch (...) {
      ROCKET_TERMINATE("`is.clear()` failed");
    }
  }

  return ret;
}

} // namespace rocket::io

// EOF
