/*
 * test-math.cc
 */

#include "rocket-gtest/testing.h"

#include "rocket/codec-global.h"

#include "rocket/S.h"
#include "rocket/except.h"
#include "rocket/math.h"

#include <random>

using namespace rocket;
using namespace rocket::math;
using namespace std;
using namespace testing;

// 'TEST' ---------------------------------------------------------------------------------------------------

TEST(math, mean) {
  {
    vector<int> v;
    EXPECT_THAT(
        [&] { mean<double>(v.begin(), v.end()); },
        ThrowsMessage<except::InvalidArgument>(HasSubstr("Parameter 'end': Check \"end > begin\" failed: Range is empty")));
  }

  {
    vector<int> v{ 1, 2, 3, 4 };
    EXPECT_EQ(mean<double>(v.begin(), v.end()), 2.5);
  }
}

TEST(math, standardDeviation) {
  vector<int> v{ 1, 2, 3, 4 };
  EXPECT_THAT(standardDeviation<double>(v.begin(), v.end()), DoubleEq(1.1180339887498949));
}

TEST(math, random) {
  random_device rd;
  mt19937 gen(rd());
  normal_distribution<double> distrib(7, 3); // Mean 7, standard deviation 3

  vector<double> v(1000000); // 1 million
  generate(v.begin(), v.end(), [&] { return distrib(gen); });

  auto m = mean<double>(v.begin(), v.end());
  EXPECT_NEAR(abs(m), 7, 0.01);

  double sigma = standardDeviation<double>(v.begin(), v.end());
  EXPECT_NEAR(sigma, 3, 0.01);
}

// EOF
