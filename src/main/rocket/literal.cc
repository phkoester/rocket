/*
 * literal.cc
 */

#include "literal.h"

#include "rocket/Exception.h"

using namespace std;

namespace rocket {

f32
operator""_f32(std_unsigned_long_long val) {
  using type = f32;
  using limits = numeric_limits<type>;
  auto ret = static_cast<type>(val);
  static_assert(static_cast<type>(numeric_limits<std_unsigned_long_long>::max()) <= limits::max());
  return ret;
}

f32
operator""_f32(std_long_double val) {
  using type = f32;
  using limits = numeric_limits<type>;
  // Check this first: casting a value outside the range of the type is undefined behavior
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  const auto ret = static_cast<type>(val);
  // Zero and subnormal values are fine. Only a nonzero value that is lost entirely has underflowed
  if (val != 0 && ret == 0) {
    throw Underflow(typeid(type));
  }
  return ret;
}

f64
operator""_f64(std_unsigned_long_long val) {
  using type = f64;
  using limits = numeric_limits<type>;
  auto ret = static_cast<type>(val);
  static_assert(static_cast<type>(numeric_limits<std_unsigned_long_long>::max()) <= limits::max());
  return ret;
}

f64
operator""_f64(std_long_double val) {
  using type = f64;
  using limits = numeric_limits<type>;
  // Check this first: casting a value outside the range of the type is undefined behavior
  if (val < limits::lowest() || val > limits::max()) {
    throw Overflow(typeid(type));
  }
  const auto ret = static_cast<type>(val);
  // Zero and subnormal values are fine. Only a nonzero value that is lost entirely has underflowed
  if (val != 0 && ret == 0) {
    throw Underflow(typeid(type));
  }
  return ret;
}

} // namespace rocket

// EOF
