#pragma once

#include "eely/math/ellipse.h"
#include "eely/math/float3.h"

namespace eely::internal {
// Represents elliptical cone with apex at origin and oriented towards +X axis.
struct elliptical_cone {
  float height{0.0F};
  ellipse ellipse{};
};

// Build elliptical cone from height and two angles around Y and around Z axes.
[[nodiscard]] elliptical_cone elliptical_cone_from_height_and_angles(float height,
                                                                     float angle_y_rad,
                                                                     float angle_z_rad);

// Return `true` if specified direction is on a cone.
bool elliptical_cone_is_direction_on(const elliptical_cone& cone, const float3& direction);

// Return `true` if specified direction is inside a cone.
[[nodiscard]] bool elliptical_cone_is_direction_inside(const elliptical_cone& cone,
                                                       const float3& direction);

// Project direction vector onto the cone.
[[nodiscard]] float3 elliptical_cone_project_direction(const elliptical_cone& cone,
                                                       float3 direction);
}  // namespace eely::internal