#pragma once

#include "eely/math/math_utils.h"

namespace eely {
// Structure that combines two floats.
// Their meaning depends on context: coordinates, coefficients etc.
struct float2 final {
  float x{0.0F};
  float y{0.0F};
};

// Return `true` if two `float2`s are exactly the same.
bool operator==(const float2& a, const float2& b);

// Return `true` if two `float2`s are not exactly the same.
bool operator!=(const float2& a, const float2& b);

// Return `true` if corresponding components are within specified epsilon.
bool float2_near(const float2& a, const float2& b, float epsilon = epsilon_default);
}  // namespace eely