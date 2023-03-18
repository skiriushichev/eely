#include "eely/math/elliptical_cone.h"

#include "eely/math/ellipse.h"
#include "eely/math/float3.h"

#include <cmath>

namespace eely::internal {
static bool is_elliptical_cone_valid(const elliptical_cone& cone)
{
  return cone.height > 0.0F && cone.ellipse.radius_x > 0.0F && cone.ellipse.radius_y > 0.0F;
}

elliptical_cone elliptical_cone_from_height_and_angles(float height,
                                                       float angle_y_rad,
                                                       float angle_z_rad)
{
  ellipse cone_ellipse{.radius_x = height * std::tan(angle_y_rad / 2.0F),
                       .radius_y = height * std::tan(angle_z_rad / 2.0F)};
  return elliptical_cone{.height = height, .ellipse = cone_ellipse};
}

bool elliptical_cone_is_direction_on(const elliptical_cone& cone, const float3& direction)
{
  // Prolong direction onto the ellipse plane.
  // Line equation for unconstrained direction is v = [tX, tY, tZ].
  // tX = cone.height -> t = cone.height / X,
  // and tV is a point an ellipse plane.

  EXPECTS(is_elliptical_cone_valid(cone));

  const float3 ellipse_plane_point{direction * (cone.height / direction.x)};

  return ellipse_is_point_on(cone.ellipse, float2{ellipse_plane_point.z, ellipse_plane_point.y});
}

bool elliptical_cone_is_direction_inside(const elliptical_cone& cone, const float3& direction)
{
  // Prolong direction onto the ellipse plane.
  // Line equation for unconstrained direction is v = [tX, tY, tZ].
  // tX = cone.height -> t = cone.height / X,
  // and tV is a point an ellipse plane.

  EXPECTS(is_elliptical_cone_valid(cone));

  if (direction.x <= 0.0F) {
    return false;
  }

  const float3 ellipse_plane_point{direction * (cone.height / direction.x)};

  return ellipse_is_point_inside(cone.ellipse,
                                 float2{ellipse_plane_point.z, ellipse_plane_point.y});
}

float3 elliptical_cone_project_direction(const elliptical_cone& cone, float3 direction)
{
  EXPECTS(is_elliptical_cone_valid(cone));

  // Prolong direction onto the ellipse plane.
  // Line equation for unconstrained direction is v = [tX, tY, tZ].
  // tX = cone.height -> t = cone.height / X,
  // and tV is a point an ellipse plane.

  if (float_near(direction.x, 0.0F)) {
    // Add some positive offset along X axis if direction is parallel to the cone's ellipse
    // Otherwise it cannot be prolonged
    direction.x = 0.01F;
  }

  // Revert direction if it's pointing away from the cone for smoother projections
  // Otherwise it would flip to the other side of a cone once X becomes negative
  direction.x = std::abs(direction.x);

  const float3 ellipse_plane_point{direction * (cone.height / direction.x)};

  // Outside of the cone
  // Project point onto the ellipse and build a new direction

  const auto [projection_point_z, projection_point_y]{
      ellipse_project_point(cone.ellipse, float2{ellipse_plane_point.z, ellipse_plane_point.y})};

  float3 projected_direction{cone.height, projection_point_y, projection_point_z};
  projected_direction = vector_normalized(projected_direction);

  return projected_direction;
}
}  // namespace eely::internal