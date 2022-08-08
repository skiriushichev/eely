#pragma once

#include <cmath>
#include <numbers>

namespace eely {
static constexpr float pi = std::numbers::pi_v<float>;
static constexpr float epsilon_default = 1E-5F;

// Return `true` if two floats are within specified epsilon.
inline bool float_near(float a, float b, float epsilon = epsilon_default)
{
  return std::abs(a - b) <= epsilon;
}

// Convert degrees to radians.
constexpr float deg_to_rad(float deg)
{
  return deg * (pi / 180.0F);
}
}  // namespace eely