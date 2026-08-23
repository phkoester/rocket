/**
 * @file Range.h
 *
 * String ranges.
 */

#pragma once

#include "rocket/math/Interval.h"

#include <vector>

namespace rocket::str {

// #Range ---------------------------------------------------------------------------------------------------

/// A string range, which is a right-open interval @f$[lower,upper)@f$.
using Range = math::RightOpenInterval<u64>;

// #Ranges --------------------------------------------------------------------------------------------------

/// String ranges.
using Ranges = std::vector<Range>;

} // namespace rocket::str

// EOF
