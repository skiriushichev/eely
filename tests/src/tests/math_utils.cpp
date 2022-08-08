#include <eely/math/math_utils.h>

#include <gtest/gtest.h>

TEST(math_utils, math_utils)
{
  using namespace eely;

  // float_near
  {
    EXPECT_TRUE(float_near(0.1F, 0.1F + epsilon_default));
    EXPECT_TRUE(float_near(1.0F, 2.0F, 1.0F));
    EXPECT_FALSE(float_near(1.0F, 1.05F, 0.049F));
    EXPECT_TRUE(float_near(-1.0F, -1.0F));
  }

  // deg_to_rad
  {
    EXPECT_TRUE(float_near(deg_to_rad(95.4F), 1.665044F));
    EXPECT_TRUE(float_near(deg_to_rad(-11.43F), -0.19949113F));
    EXPECT_TRUE(float_near(deg_to_rad(247.56), 4.3207371F));
  }
}