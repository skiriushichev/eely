#pragma once

#include "eely/clip_utils.h"
#include "eely/float3.h"
#include "eely/math_utils.h"
#include "eely/quaternion.h"
#include "eely/transform.h"

#include <gtest/gtest.h>

namespace eely {
// Seed for tests that use random number generators
// So that all values used in a test were reproducable
static constexpr int seed = 30091990;

inline void expect_float3_near(const float3& a, const float3& b, float epsilon = epsilon_default)
{
  EXPECT_TRUE(float3_near(a, b, epsilon));
}

inline void expect_float3_not_near(const float3& a,
                                   const float3& b,
                                   float epsilon = epsilon_default)
{
  EXPECT_FALSE(float3_near(a, b, epsilon));
}

inline void expect_quaternion_near(const quaternion& a,
                                   const quaternion& b,
                                   float epsilon = epsilon_default)
{
  const bool near_as_is{quaternion_near(a, b, epsilon)};
  if (!near_as_is) {
    // Negated quaternion represents the same rotation, check if this is the case
    const bool near_negated{quaternion_near(a, quaternion{-b.x, -b.y, -b.z, -b.w}, epsilon)};
    if (!near_negated) {
      // This is just to fail the test
      EXPECT_TRUE(quaternion_near(a, b, epsilon));
    }
  }
}

inline void expect_transform_near(const transform& a,
                                  const transform& b,
                                  float epsilon = epsilon_default)
{
  expect_float3_near(a.translation, b.translation, epsilon);
  expect_quaternion_near(a.rotation, b.rotation, epsilon);
  expect_float3_near(a.scale, b.scale, epsilon);
}

inline float calculate_acceptable_quantize_error(const eely::float_quantize_params& params)
{
  // `float_quantize` truncates float quantized value to `uint16_t` now,
  // thus maximum error is one quant, plus some epsilon for floating point accuracy
  const gsl::index quants_count{1 << params.bits_count};
  const float value_per_quant{params.range_length / gsl::narrow_cast<float>(quants_count - 1)};
  EXPECT_TRUE(std::isfinite(value_per_quant));
  return value_per_quant + eely::epsilon_default;
}
}  // namespace eely