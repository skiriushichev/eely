#include "eely/math/quaternion.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/math/float3.h"
#include "eely/math/math_utils.h"

#include <cmath>

namespace eely {
const quaternion quaternion::identity{0.0F, 0.0F, 0.0F, 1.0F};

quaternion operator*(const quaternion& q0, const quaternion& q1)
{
  const float3 v0{q0.x, q0.y, q0.z};
  const float3 v1{q1.x, q1.y, q1.z};

  const float3 v{q0.w * v1 + q1.w * v0 + vector_cross(v0, v1)};
  const float w{q0.w * q1.w - vector_dot(v0, v1)};

  return quaternion{v.x, v.y, v.z, w};
}

bool operator==(const quaternion& q0, const quaternion& q1)
{
  return q0.x == q1.x && q0.y == q1.y && q0.z == q1.z && q0.w == q1.w;
}

bool operator!=(const quaternion& q0, const quaternion& q1)
{
  return !(q0 == q1);
}

float quaternion_length(const quaternion& q)
{
  return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

quaternion quaternion_normalized(const quaternion& q)
{
  const float length{quaternion_length(q)};
  EXPECTS(length > 0.0F);

  const float length_inversed{1.0F / length};

  return quaternion{q.x * length_inversed, q.y * length_inversed, q.z * length_inversed,
                    q.w * length_inversed};
}

quaternion quaternion_inverse(const quaternion& q)
{
  return quaternion{-q.x, -q.y, -q.z, q.w};
}

quaternion quaternion_from_yaw_pitch_roll_intrinsic(const float yaw,
                                                    const float pitch,
                                                    const float roll)
{
  const float cos_yaw = std::cos(yaw / 2.0F);
  const float sin_yaw = std::sin(yaw / 2.0F);
  const float cos_pitch = std::cos(pitch / 2.0F);
  const float sin_pitch = std::sin(pitch / 2.0F);
  const float cos_roll = std::cos(roll / 2.0F);
  const float sin_roll = std::sin(roll / 2.0F);

  return (quaternion{cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll,
                     sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll,
                     cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll,
                     cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll});
}

float3 quaternion_to_yaw_pitch_roll_intrinsic(const quaternion& q)
{
  float yaw{0.0F};
  float pitch{0.0F};
  float roll{0.0F};

  float sp{-2.0F * (q.y * q.z - q.w * q.x)};

  // Gimbal lock check
  if (std::abs(sp) > 0.9999F) {
    // Looking straight up or down
    pitch = (pi / 2.0F) * sp;

    // Compute yaw, roll is zero
    yaw = std::atan2(-q.x * q.z + q.w * q.y, 0.5F - q.y * q.y - q.z * q.z);
    roll = 0.0F;
  }
  else {
    pitch = std::asin(sp);
    yaw = std::atan2(q.x * q.z + q.w * q.y, 0.5F - q.x * q.x - q.y * q.y);
    roll = std::atan2(q.x * q.y + q.w * q.z, 0.5F - q.x * q.x - q.z * q.z);
  }

  return float3{yaw, pitch, roll};
}

quaternion quaternion_from_axis_angle(const float x,
                                      const float y,
                                      const float z,
                                      const float angle_rad)
{
  float3 axis{x, y, z};
  axis = vector_normalized(axis);

  const float s{std::sin(angle_rad / 2.0F)};
  const float c{std::cos(angle_rad / 2.0F)};

  return quaternion{axis.x * s, axis.y * s, axis.z * s, c};
}

std::pair<float3, float> quaternion_to_axis_angle(const quaternion& q)
{
  const float length_xyz{std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z)};

  if (length_xyz <= epsilon_default) {
    return {float3::x_axis, 0.0F};
  }

  std::pair<float3, float> result;

  const float length_xyz_inversed{1.0F / length_xyz};
  result.first =
      float3{q.x * length_xyz_inversed, q.y * length_xyz_inversed, q.z * length_xyz_inversed};

  const float atan{q.w < 0.0F ? std::atan2(-length_xyz, -q.w) : std::atan2(length_xyz, q.w)};
  result.second = 2.0F * atan;

  return result;
}

quaternion quaternion_slerp(const quaternion& q0, const quaternion& q1, float t)
{
  float cos_angle{q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w};

  quaternion q1_shortest;
  if (cos_angle < 0.0F) {
    cos_angle = -cos_angle;
    q1_shortest = quaternion{-q1.x, -q1.y, -q1.z, -q1.w};
  }
  else {
    q1_shortest = q1;
  }

  float k0{1.0F - t};
  float k1{t};
  if (cos_angle < 0.95F) {
    const float angle{std::acos(cos_angle)};
    const float sin_angle{std::sin(angle)};

    const float sin_inversed{1.0F / sin_angle};

    k0 = std::sin(k0 * angle) * sin_inversed;
    k1 = std::sin(k1 * angle) * sin_inversed;
  }

  return quaternion_normalized(
      quaternion{k0 * q0.x + k1 * q1_shortest.x, k0 * q0.y + k1 * q1_shortest.y,
                 k0 * q0.z + k1 * q1_shortest.z, k0 * q0.w + k1 * q1_shortest.w});
}

quaternion quaternion_extract(const quaternion& q, const float3& axis)
{
  EXPECTS(float_near(quaternion_length(q), 1.0F));
  EXPECTS(float_near(vector_length(axis), 1.0F));

  float3 projection{vector_dot({q.x, q.y, q.z}, axis) * axis};

  quaternion result{projection.x, projection.y, projection.z, q.w};
  if (float_near(quaternion_length(result), 0.0F)) {
    result = quaternion::identity;
  }
  else {
    result = quaternion_normalized(result);
  }

  if (vector_dot(axis, {q.x, q.y, q.z}) < 0.0) {
    // Ensure that result axis points towards `direction` and not in its opposite
    result.x = -result.x;
    result.y = -result.y;
    result.z = -result.z;
    result.w = -result.w;
  }

  if (!float_near(quaternion_to_axis_angle(result).second, 0.0F)) {
    float3 axis_extracted{quaternion_to_axis_angle(result).first};
    EXPECTS(float3_near(axis_extracted, axis));
  }

  return result;
}

quaternion quaternion_shortest_arc(const float3& from, const float3& to)
{
  const float dot_product{vector_dot(from, to)};
  if (float_near(dot_product, 1.0F)) {
    // Return an identity quaternion for same direction collinear vectors.
    return quaternion::identity;
  }

  if (float_near(dot_product, -1.0F)) {
    // Find any non-collinear vector to find any orthogonal vector.
    float3 non_collinear_vector{0.0F, 1.0F, 0.0F};
    if (float_near(std::abs(vector_dot(from, non_collinear_vector)), 1.0F)) {
      non_collinear_vector = float3{1.0F, 0.0F, 0.0F};
    }

    // This is an orthogonal vector we will twist around.
    const float3 orthogonal_vector{vector_normalized(vector_cross(from, non_collinear_vector))};

    // Twist around `orthogonal_vector` by 180 degrees.
    return quaternion{orthogonal_vector.x, orthogonal_vector.y, orthogonal_vector.z, 0.0F};
  }

  const float3 axis{vector_cross(from, to)};
  const quaternion unnormalized{quaternion{axis.x, axis.y, axis.z, dot_product + 1.0F}};
  return quaternion_normalized(unnormalized);
}

bool quaternion_near(const quaternion& q0, const quaternion& q1, const float epsilon)
{
  return float_near(q0.x, q1.x, epsilon) && float_near(q0.y, q1.y, epsilon) &&
         float_near(q0.z, q1.z, epsilon) && float_near(q0.w, q1.w, epsilon);
}

float3 vector_rotate(const float3& v, const quaternion& q)
{
  const quaternion v_ext{v.x, v.y, v.z, 0.0F};
  const quaternion v_rotated_ext{q * v_ext * quaternion_inverse(q)};
  return float3{v_rotated_ext.x, v_rotated_ext.y, v_rotated_ext.z};
}

namespace internal {
void bit_writer_write(bit_writer& writer, const quaternion& value)
{
  bit_writer_write(writer, value.x);
  bit_writer_write(writer, value.y);
  bit_writer_write(writer, value.z);
  bit_writer_write(writer, value.w);
}
}  // namespace internal
}  // namespace eely