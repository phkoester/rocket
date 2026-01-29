/*
 * literal.cc
 */

#include "literal.h"

#include "rocket/Exception.h"

using namespace std;

namespace rocket {

f32
operator""_f32(std_unsigned_long_long_int val) {
  using type = f32;
  using limits = numeric_limits<type>;
  auto ret = static_cast<type>(val);
  static_assert(numeric_limits<std_unsigned_long_long_int>::max() <= limits::max());
  return ret;
}

f32
operator""_f32(std_long_double val) {
  using type = f32;
  using limits = numeric_limits<type>;
  if (fabs(val) < limits::min()) {
    throw Underflow(typeid(type));
  }
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  return static_cast<type>(val);
}

f64
operator""_f64(std_unsigned_long_long_int val) {
  using type = f64;
  using limits = numeric_limits<type>;
  auto ret = static_cast<type>(val);
  static_assert(numeric_limits<std_unsigned_long_long_int>::max() <= limits::max());
  return ret;
}

f64
operator""_f64(std_long_double val) {
  using type = f64;
  using limits = numeric_limits<type>;
  if (fabs(val) < limits::min()) {
    throw Underflow(typeid(type));
  }
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  return static_cast<type>(val);
}

#ifdef ROCKET_HAS_128

f128
operator""_f128(std_unsigned_long_long_int val) {
  using type = f128;
  using limits = numeric_limits<type>;
  auto ret = static_cast<type>(val);
  static_assert(numeric_limits<std_unsigned_long_long_int>::max() <= limits::max());
  return ret;
}

f128
operator""_f128(std_long_double val) {
  using type = f128;
  static_assert(sizeof(type) == sizeof(std_long_double));
  return static_cast<type>(val);
}

#endif // ROCKET_HAS_128

} // namespace rocket

// EOF
