/**
 * @file math.h
 *
 * Mathematical utilities.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/rocket.h"

#include <algorithm>
#include <numeric>
#include <vector>

namespace rocket::math {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Calculates the average mean for the values in the half-open interval @f$[begin,end)@f$.
 *
 * To avoid overflow, this function calculates a <em>cumulative moving average</em>. Let the values be
 * @f$(x_{1},...,x_{N})@f$, this functions returns
 *
 * @f[
 * \overline{x} = \frac{\sum_{i=1}^N x_i}{N}
 * @f]
 *
 * @tparam T the element type
 * @tparam It the iterator type
 * @param begin the beginning of the range, inclusive
 * @param end the end of the range, exclusive
 * @return the mean
 */
 template<typename T, typename It> requires FloatingPoint<T>
 T
 mean(It begin, It end) {
   ROCKET_CHECK(end, end > begin, "Range is empty");

   // Calculating CMA = Cumulative Moving Average
   T ret = 0;
   size_t n = 1;
   for (auto it = begin; it != end; ++it)
     ret += (*it - ret) / n++;
   return ret;
 }

 /**
  * Calculates the standard deviation for the values in the half-open interval @f$[begin,end)@f$.
  *
  * Let the values be @f$(x_{1},...,x_{N})@f$, this functions returns
  *
  * @f[
  * \sigma = \sqrt{\frac{1}{N} \sum_{i=1}^N (x_i - \overline{x})^2}
  * @f]
  *
  * @tparam T the element type
  * @tparam It the iterator type
  * @param begin the beginning of the range, inclusive
  * @param end the end of the range, exclusive
  * @return the standard deviation
  */
 template<typename T, typename It> requires FloatingPoint<T>
 T
 standardDeviation(It begin, It end) {
   T m = mean<T>(begin, end);

   auto n = std::distance(begin, end); // We know this is > 0
   std::vector<T> diff;
   diff.reserve(n);
   std::for_each(begin, end, [&](auto&& x) { diff.push_back(std::forward<decltype(x)>(x) - m); });

   T sumOfSquares = std::inner_product(diff.begin(), diff.end(), diff.begin(), static_cast<T>(0));
   return std::sqrt(sumOfSquares / n);
 }

 } // namespace rocket::math

// EOF
