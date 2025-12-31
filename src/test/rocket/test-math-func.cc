/*
 * test-math-func.cc
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/Exception.h"
#include "rocket/math-func.h"
#include "rocket/random.h"

using namespace rocket;
using namespace rocket::math;
using namespace std;
using namespace testing;

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(math, mean) {
  {
    vector<int> v;
    EXPECT_THAT(
        [&] { mean<double>(v.begin(), v.end()); },
        ThrowsMessage<InvalidArgument>(HasSubstr("Parameter `end`: Check `end > begin` failed: Range is empty")));
  }

  {
    vector<int> v{ 1, 2, 3, 4 };
    EXPECT_EQ(mean<double>(v.begin(), v.end()), 2.5);
  }
}

TEST(math, meanAndStandardDeviation) {
  auto gen = random::gen();
  normal_distribution<double> distrib(7, 3); // Mean 7, standard deviation 3

  vector<double> v(1'000'000); // 1 million
  generate(v.begin(), v.end(), [&]{ return distrib(gen); });

  auto mu = mean<double>(v.begin(), v.end());
  EXPECT_NEAR(mu, 7, 0.01);

  double sigma = standardDeviation<double>(v.begin(), v.end());
  EXPECT_NEAR(sigma, 3, 0.01);
}

TEST(math, standardDeviation) {
  vector<int> v{ 1, 2, 3, 4 };
  EXPECT_THAT(standardDeviation<double>(v.begin(), v.end()), DoubleEq(1.1180339887498949));
}

// EOF
