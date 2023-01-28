#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/math/math_utils.h"

namespace eely {
// Structure that combines three floats.
// Their meaning depends on context: point, displacement, coefficients etc.
struct float3 final {
  static const float3 zeroes;
  static const float3 ones;

  float x{0.0F};
  float y{0.0F};
  float z{0.0F};
};

// Return component-wise sum of two `float3`s.
float3 operator+(const float3& a, const float3& b);

// Return `float3` with another `float3` added component-wise.
float3& operator+=(float3& a, const float3& b);

// Return component-wise subtraction of two `float3`s.
float3 operator-(const float3& a, const float3& b);

// Return `float3` with another `float3` subtracted component-wise.
float3& operator-=(float3& a, const float3& b);

// Return `float3` with all its component negated.
float3 operator-(const float3& a);

// Return specified `float3` with each component multiplied by a scalar.
float3 operator*(const float3& a, float scalar);

// Return specified `float3` with each component multiplied by a scalar.
float3 operator*(float scalar, const float3& a);

// Return component-wise multiplication of `float3`s.
float3 operator*(const float3& a, const float3& b);

// Return `true` if two `float3`s are exactly the same.
bool operator==(const float3& a, const float3& b);

// Return `true` if two `float3`s are not exactly the same.
bool operator!=(const float3& a, const float3& b);

// Linearly interpolate between two `float3`s.
float3 float3_lerp(const float3& a, const float3& b, float t);

// Return `true` if corresponding components are within specified epsilon.
bool float3_near(const float3& a, const float3& b, float epsilon = epsilon_default);

// Return normalized vector.
float3 vector_normalized(const float3& a);

// Return dot product of two vectors.
float vector_dot(const float3& a, const float3& b);

// Return cross product of two vectors.
float3 vector_cross(const float3& a, const float3& b);

namespace internal {
// Return `float3` value read from a memory buffer.
template <>
float3 bit_reader_read(bit_reader& reader);

// Write `float3 into a memory buffer.
void bit_writer_write(bit_writer& writer, const float3& value);
}  // namespace internal

// Implementation

namespace internal {
template <>
inline float3 bit_reader_read(bit_reader& reader)
{
  return float3{.x = bit_reader_read<float>(reader),
                .y = bit_reader_read<float>(reader),
                .z = bit_reader_read<float>(reader)};
}
}  // namespace internal
}  // namespace eely