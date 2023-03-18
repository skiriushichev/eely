#pragma once

#include "eely/math/float2.h"

namespace eely::internal {
// Represents ellipse aligned with X and Y axes.
// `radius_x` and `radius_y` are also `a` and `b`
// in general ellipse equation:
// `(x * x) / (a * a) + (y * y) / (b * b) = 1`
struct ellipse final {
  float radius_x{0.0F};
  float radius_y{0.0F};
};

// Calculate point on an ellipse from a given positive angle.
[[nodiscard]] float2 ellipse_point_from_angle(const ellipse& ellipse, float angle_rad);

// Calculate projection of a given point on an ellipse,
// e.g. return another point that lies on an ellipse that is closest to the given point.
[[nodiscard]] float2 ellipse_project_point(const ellipse& ellipse, const float2& point);

// Return `true` if point is on a given ellipse.
[[nodiscard]] bool ellipse_is_point_on(const ellipse& ellipse, const float2& point);

// Return `true` if point is inside a given ellipse.
[[nodiscard]] bool ellipse_is_point_inside(const ellipse& ellipse, const float2& point);
}  // namespace eely::internal