#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/float3.h"
#include "eely/math_utils.h"

#include <utility>

namespace eely {
// Represents rotation quaternion.
struct quaternion final {
  static const quaternion identity;

  float x{0.0F};
  float y{0.0F};
  float z{0.0F};
  float w{1.0F};
};

// Return result of two quaternions multiplication.
// `q0 * q1` first does `q1` rotation in global coordinate space,
// and then `q0` rotation in global coordinate space.
// Or can be seen as `q0` rotation applied in global coordinate space,
// and then `q1` rotation applied in coordinate space resulting from `q0`.
quaternion operator*(const quaternion& q0, const quaternion& q1);

// Return `true` if two quaternions are exactly the same.
bool operator==(const quaternion& q0, const quaternion& q1);

// Return `true` if two quaternions are not exactly the same.
bool operator!=(const quaternion& q0, const quaternion& q1);

// Return length of a quaternion.
float quaternion_length(const quaternion& q);

// Normalize a quaternion.
quaternion quaternion_normalized(const quaternion& q);

// Return inverse of a normalized quaternion.
// Inversed quaternion undoes rotation made by the original.
quaternion quaternion_inverse(const quaternion& q);

// Return `quaternion` which combines rotations around three axes.
// Order of rotations is: yaw, pitch and then roll.
// Each rotation is made around bodial axis resulting from previous rotation
// and not around fixed axis ("intrinsic").
quaternion quaternion_from_yaw_pitch_roll_intrinsic(float yaw, float pitch, float roll);

// Return `quaternion` which represents rotation
// converted from axis-angle representation.
quaternion quaternion_from_axis_angle(float x, float y, float z, float angle_rad);

// Convert quaternion to rotation axis and angle representation.
std::pair<float3, float> quaternion_to_axis_angle(const quaternion& q);

// Spherically interpolate between two quaternions based on parameter `t`.
quaternion quaternion_slerp(const quaternion& q0, const quaternion& q1, float t);

// Return `true` if corresponding components of two quaternions
// are within specified epsilon.
bool quaternion_near(const quaternion& q0, const quaternion& q1, float epsilon = epsilon_default);

// Serialize `quaternion` into specified memory buffer.
void quaternion_serialize(const quaternion& q, bit_writer& writer);

// Deserialize `quaternion` from specified memory buffer.
quaternion quaternion_deserialize(bit_reader& reader);

// Return vector rotated by a quaternion.
float3 vector_rotate(const float3& v, const quaternion& q);
}  // namespace eely