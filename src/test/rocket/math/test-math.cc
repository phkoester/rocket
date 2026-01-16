/*
 * test-math.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"
#include "rocket/math/math.h"
#include "rocket/math/random.h"

using namespace rocket::math;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(math, mean) {
  {
    vector<i32> v;
    EXPECT_THAT(
        [&] { mean<f64>(v.begin(), v.end()); },
        ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `end`: Check `end > begin` failed: Range is empty")));
  }

  {
    vector<i32> v{ 1, 2, 3, 4 };
    EXPECT_EQ(mean<f64>(v.begin(), v.end()), 2.5);
  }
}

TEST(math, meanAndStandardDeviation) {
  auto gen = math::gen();
  normal_distribution<f64> distrib(7, 3); // Mean 7, standard deviation 3

  vector<f64> v(1'000'000); // 1 million
  generate(v.begin(), v.end(), [&]{ return distrib(gen); });

  auto mu = mean<f64>(v.begin(), v.end());
  EXPECT_NEAR(mu, 7, 0.01);

  f64 sigma = standardDeviation<f64>(v.begin(), v.end());
  EXPECT_NEAR(sigma, 3, 0.01);
}

TEST(math, standardDeviation) {
  vector<i32> v{ 1, 2, 3, 4 };
  EXPECT_THAT(standardDeviation<f64>(v.begin(), v.end()), DoubleEq(1.1180339887498949));
}

// EOF
