#include "eely/math/ellipse.h"

#include "eely/base/assert.h"
#include "eely/math/math_utils.h"

#include <cmath>

namespace eely::internal {
static bool is_ellipse_valid(const ellipse& ellipse)
{
  return ellipse.radius_x > 0.0F && ellipse.radius_y > 0.0F;
}

float2 ellipse_point_from_angle(const ellipse& ellipse, float angle_rad)
{
  // Equation of a line from origin passing through the point is `y = x * tan(angle_rad)`
  // Substitute `y` in general ellipse equation to find `x` value and then calculate y value

  EXPECTS(is_ellipse_valid(ellipse));
  EXPECTS(angle_rad >= 0.0F);

  float x_sign{1.0F};
  float y_sign{1.0F};

  angle_rad = std::fmod(angle_rad, pi * 2.0F);
  EXPECTS(angle_rad <= pi * 2.0F);
  if (angle_rad > pi / 2.0F) {
    if (angle_rad <= pi) {
      x_sign = -1.0F;
    }
    else if (angle_rad <= 3.0F * pi / 2.0F) {
      x_sign = -1.0F;
      y_sign = -1.0F;
    }
    else {
      y_sign = -1.0F;
    }
  }

  const float tan_value{std::tan(angle_rad)};
  const float tan_value_sqr{tan_value * tan_value};

  const float radius_x_sqr = ellipse.radius_x * ellipse.radius_x;
  const float radius_y_sqr = ellipse.radius_y * ellipse.radius_y;

  float x{std::sqrtf(std::fmax(
      0.0F, (radius_x_sqr * radius_y_sqr) / (radius_y_sqr + tan_value_sqr * radius_x_sqr)))};

  float y{std::sqrtf(
      std::fmax(0.0F, ((radius_x_sqr * radius_y_sqr) - x * x * radius_y_sqr) / radius_x_sqr))};
  return float2{x_sign * x, y_sign * y};
}

float2 ellipse_project_point(const ellipse& ellipse, const float2& point)
{
  // Based on "Algorithms of projection of a point onto an ellipsoid" by Yu. N. Kiseliov
  // https://link.springer.com/article/10.1007/BF02333413
  // Use Newton's method to find the root

  // More info on this problem:
  // https://www.geometrictools.com/Documentation/DistancePointEllipseEllipsoid.pdf

  EXPECTS(is_ellipse_valid(ellipse));

  float2 result;

  if (float_near(point.x, 0.0F)) {
    result.x = 0;
    result.y = std::copysign(ellipse.radius_y, point.y);
  }
  else if (float_near(point.y, 0.0F)) {
    result.x = std::copysign(ellipse.radius_x, point.x);
    result.y = 0;
  }
  else {
    float point_x_sqr{point.x * point.x};
    float point_y_sqr{point.y * point.y};

    float radius_x_sqr{ellipse.radius_x * ellipse.radius_x};
    float radius_y_sqr{ellipse.radius_y * ellipse.radius_y};
    float radius_max_sqr{std::fmax(radius_x_sqr, radius_y_sqr)};

    float h_min{std::sqrt((point_x_sqr * radius_x_sqr * radius_x_sqr +
                           point_y_sqr * radius_y_sqr * radius_y_sqr) /
                          radius_max_sqr) -
                radius_max_sqr};

    h_min = std::fmax(h_min, (std::fabs(point.x) - ellipse.radius_x) * ellipse.radius_x);
    h_min = std::fmax(h_min, (std::fabs(point.y) - ellipse.radius_y) * ellipse.radius_y);

    if (point_x_sqr / radius_x_sqr + point_y_sqr / radius_y_sqr > 1.0F && h_min < 0.0F) {
      h_min = 0.0F;
    }

    float h_prev{h_min};
    float h_current{h_min};

    static constexpr int max_iterations{20};
    int iterations{0};
    do {
      float sum_x{radius_x_sqr + h_current};
      float sum_y{radius_y_sqr + h_current};

      float x{std::powf(point.x / sum_x, 2) * radius_x_sqr};
      float y{std::powf(point.y / sum_y, 2) * radius_y_sqr};

      h_prev = h_current;

      float f{1.0F - (x + y)};
      float df{2.0F * (x / sum_x + y / sum_y)};

      h_current -= f / df;

      if (h_current < h_min) {
        h_current = (h_prev + h_min) / 2.0F;
        continue;
      }

      ++iterations;
      if (iterations > max_iterations) {
        break;
      }

    } while (h_current > h_prev);

    result.x = point.x * radius_x_sqr / (radius_x_sqr + h_current);
    result.y = point.y * radius_y_sqr / (radius_y_sqr + h_current);
  }

  return result;
}

bool ellipse_is_point_on(const ellipse& ellipse, const float2& point)
{
  // General equation: (x * x) / (a * a) + (y * y) / (b * b) = 1.

  EXPECTS(is_ellipse_valid(ellipse));

  const float x_sqr{point.x * point.x};
  const float y_sqr{point.y * point.y};
  const float radius_x_sqr{ellipse.radius_x * ellipse.radius_x};
  const float radius_y_sqr{ellipse.radius_y * ellipse.radius_y};

  return float_near(x_sqr / radius_x_sqr + y_sqr / radius_y_sqr, 1.0F);
}

bool ellipse_is_point_inside(const ellipse& ellipse, const float2& point)
{
  // General equation: (x * x) / (a * a) + (y * y) / (b * b) = 1.
  // If <= 1, it means point is inside.

  EXPECTS(is_ellipse_valid(ellipse));

  const float x_sqr{point.x * point.x};
  const float y_sqr{point.y * point.y};
  const float radius_x_sqr{ellipse.radius_x * ellipse.radius_x};
  const float radius_y_sqr{ellipse.radius_y * ellipse.radius_y};

  return (x_sqr / radius_x_sqr + y_sqr / radius_y_sqr <= 1.0F + epsilon_default);
}
}  // namespace eely::internal