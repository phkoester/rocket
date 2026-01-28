/*
 * literal.cc
 */

#include "literal.h"

#include "rocket/Exception.h"

namespace rocket {

f32
operator""_f32(std_long_double val) {
  using type = f32;
  using limits = std::numeric_limits<type>;
  if (std::fabs(val) < limits::min()) {
    throw Underflow(typeid(type));
  }
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  return static_cast<type>(val);
}

f64
operator""_f64(std_long_double val) {
  using type = f64;
  using limits = std::numeric_limits<type>;
  if (std::fabs(val) < limits::min()) {
    throw Underflow(typeid(type));
  }
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  return static_cast<type>(val);
}

#ifdef ROCKET_HAS_128

f128
operator""_f128(std_long_double val) {
  using type = f128;
  static_assert(sizeof(type) == sizeof(std_long_double));
  return static_cast<type>(val);
}

#endif // ROCKET_HAS_128

} // namespace rocket

// EOF
