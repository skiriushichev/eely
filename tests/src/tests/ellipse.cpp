#include "tests/test_utils.h"

#include <eely/math/ellipse.h>
#include <eely/math/float2.h>

#include <gtest/gtest.h>

void test_ellipse(const float radius_x, const float radius_y)
{
  using namespace eely;
  using namespace eely::internal;

  ellipse ellipse{.radius_x = radius_x, .radius_y = radius_y};

  // `ellipse_point_from_angle`

  EXPECT_TRUE(float2_near(ellipse_point_from_angle(ellipse, 0.0F), (float2{radius_x, 0.0F})));
  EXPECT_TRUE(float2_near(ellipse_point_from_angle(ellipse, pi / 2.0F), (float2{0.0F, radius_y})));
  EXPECT_TRUE(float2_near(ellipse_point_from_angle(ellipse, pi), (float2{-radius_x, 0.0F})));
  EXPECT_TRUE(
      float2_near(ellipse_point_from_angle(ellipse, 3.0F * pi / 2.0F), (float2{0.0F, -radius_y})));

  EXPECT_TRUE(
      float2_near(ellipse_point_from_angle(ellipse, pi * 2.0F + 0.0F), (float2{radius_x, 0.0F})));
  EXPECT_TRUE(float2_near(ellipse_point_from_angle(ellipse, pi * 2.0F + pi / 2.0F),
                          (float2{0.0F, radius_y})));
  EXPECT_TRUE(
      float2_near(ellipse_point_from_angle(ellipse, pi * 2.0F + pi), (float2{-radius_x, 0.0F})));
  EXPECT_TRUE(float2_near(ellipse_point_from_angle(ellipse, pi * 2.0F + 3.0F * pi / 2.0F),
                          (float2{0.0F, -radius_y})));

  // `ellipse_is_point_on`

  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_point_from_angle(ellipse, 0.45F)));
  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_point_from_angle(ellipse, 1.214F)));
  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_point_from_angle(ellipse, 2.15F)));
  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_point_from_angle(ellipse, 10.45F)));

  // `ellipse_is_point_inside`

  EXPECT_TRUE(ellipse_is_point_inside(ellipse, ellipse_point_from_angle(ellipse, 0.45F)));
  EXPECT_TRUE(ellipse_is_point_inside(ellipse, ellipse_point_from_angle(ellipse, 1.214F)));
  EXPECT_TRUE(ellipse_is_point_inside(ellipse, ellipse_point_from_angle(ellipse, 2.15F)));
  EXPECT_TRUE(ellipse_is_point_inside(ellipse, ellipse_point_from_angle(ellipse, 10.45F)));
  EXPECT_FALSE(ellipse_is_point_inside(ellipse, float2{radius_x, radius_y}));
  EXPECT_FALSE(ellipse_is_point_inside(ellipse, float2{-radius_x, radius_y}));
  EXPECT_FALSE(ellipse_is_point_inside(ellipse, float2{radius_x, -radius_y}));
  EXPECT_FALSE(ellipse_is_point_inside(ellipse, float2{-radius_x, -radius_y}));

  // `ellipse_project_point`

  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{0.0F, 0.0F})));
  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{radius_x, 0.0F})));
  EXPECT_TRUE(
      ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{radius_x / 2.0F, 0.0F})));
  EXPECT_TRUE(
      ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{-radius_x / 2.0F, 0.0F})));
  EXPECT_TRUE(ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{0.0F, radius_y})));
  EXPECT_TRUE(
      ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{0.0F, radius_y / 2.0F})));
  EXPECT_TRUE(
      ellipse_is_point_on(ellipse, ellipse_project_point(ellipse, float2{0.0F, -radius_y / 2.0F})));

  EXPECT_TRUE(ellipse_is_point_on(
      ellipse, ellipse_project_point(ellipse, float2{2 * radius_x, 2 * radius_y})));
  EXPECT_TRUE(ellipse_is_point_on(
      ellipse, ellipse_project_point(ellipse, float2{-2 * radius_x, 2 * radius_y})));
  EXPECT_TRUE(ellipse_is_point_on(
      ellipse, ellipse_project_point(ellipse, float2{2 * radius_x, -2 * radius_y})));
  EXPECT_TRUE(ellipse_is_point_on(
      ellipse, ellipse_project_point(ellipse, float2{-2 * radius_x, -2 * radius_y})));
}

TEST(ellipse, ellipse)
{
  using namespace eely;
  using namespace eely::internal;

  test_ellipse(5.0F, 10.0F);
  test_ellipse(20.0F, 2.0F);
  test_ellipse(4.0F, 4.0F);
  test_ellipse(20.0F, 4.0F);
  test_ellipse(4.0F, 20.0F);
}