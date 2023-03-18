#include "tests/test_utils.h"

#include <eely/math/ellipse.h>
#include <eely/math/elliptical_cone.h>
#include <eely/math/float2.h>
#include <eely/math/float3.h>
#include <eely/math/quaternion.h>

#include <gtest/gtest.h>

eely::float3 dir_from_angles(const float angle_y_rad, const float angle_z_rad)
{
  using namespace eely;

  const quaternion q{quaternion_from_yaw_pitch_roll_intrinsic(angle_y_rad, angle_z_rad, 0.0F)};
  return vector_rotate(float3{1.0F, 0.0F, 0.0F}, q);
}

void test_elliptical_cone(const float height, const float angle_y_rad, const float angle_z_rad)
{
  using namespace eely;
  using namespace eely::internal;

  elliptical_cone cone{elliptical_cone_from_height_and_angles(height, angle_y_rad, angle_z_rad)};

  // `elliptical_cone_is_direction_on`

  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, 0.0F, cone.ellipse.radius_x})));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, 0.0F, -cone.ellipse.radius_x})));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, cone.ellipse.radius_y, 0.0F})));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, -cone.ellipse.radius_y, 0.0F})));

  EXPECT_FALSE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, 0.0F, cone.ellipse.radius_x + 0.1F})));
  EXPECT_FALSE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, 0.0F, -cone.ellipse.radius_x - 0.1F})));
  EXPECT_FALSE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, cone.ellipse.radius_y + 0.1F, 0.0F})));
  EXPECT_FALSE(elliptical_cone_is_direction_on(
      cone, vector_normalized(float3{cone.height, -cone.ellipse.radius_y - 0.1F, 0.0F})));

  // `elliptical_cone_is_direction_inside`

  EXPECT_TRUE(elliptical_cone_is_direction_inside(cone, dir_from_angles(0.0F, 0.0F)));
  EXPECT_TRUE(elliptical_cone_is_direction_inside(cone, dir_from_angles(angle_y_rad / 2.0F, 0.0F)));
  EXPECT_TRUE(elliptical_cone_is_direction_inside(cone, dir_from_angles(angle_y_rad / 4.0F, 0.0F)));
  EXPECT_TRUE(elliptical_cone_is_direction_inside(cone, dir_from_angles(0.0F, angle_z_rad / 2.0F)));
  EXPECT_TRUE(elliptical_cone_is_direction_inside(cone, dir_from_angles(0.0F, angle_z_rad / 4.0F)));

  // `elliptical_cone_project_direction`

  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(cone, dir_from_angles(0.0F, 0.0F))));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(cone, dir_from_angles(angle_y_rad, 0.0F))));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(cone, dir_from_angles(0.0F, angle_z_rad))));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(cone, dir_from_angles(angle_y_rad, angle_z_rad))));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(cone, dir_from_angles(-angle_y_rad, -angle_z_rad))));
  EXPECT_TRUE(elliptical_cone_is_direction_on(
      cone, elliptical_cone_project_direction(
                cone, dir_from_angles(angle_y_rad * 5.0F, -angle_z_rad * 145.0F))));
}

TEST(elliptical_cone, elliptical_cone)
{
  using namespace eely;
  using namespace eely::internal;

  test_elliptical_cone(1.0F, pi / 2.0F, pi / 2.0F);
  test_elliptical_cone(10.0F, pi / 2.0F, pi / 16.0F);
  test_elliptical_cone(100.0F, pi / 32.0F, pi / 8.0F);
  test_elliptical_cone(0.01F, pi / 32.0F, pi / 8.0F);
  test_elliptical_cone(5.0F, pi * 0.95F, pi * 0.95F);
  test_elliptical_cone(0.1F, pi * 0.03F, pi * 0.05F);
}