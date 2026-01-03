/**
 * @file Range.h
 *
 * String ranges.
 */

#pragma once

#include "rocket/math/Interval.h"

#include <vector>

namespace rocket::str {

// `Range` --------------------------------------------------------------------------------------------------

/**
 * A string range.
 */
using Range = math::RightOpenInterval<size_t>;

// `Ranges` -------------------------------------------------------------------------------------------------

/**
 * String ranges.
 */
using Ranges = std::vector<Range>;

} // namespace rocket::str

// EOF
