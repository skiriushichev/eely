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
  const quaternion qy{quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, yaw)};
  const quaternion qp{quaternion_from_axis_angle(1.0F, 0.0F, 0.0F, pitch)};
  const quaternion qr{quaternion_from_axis_angle(0.0F, 0.0F, 1.0F, roll)};

  return qy * qp * qr;
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
    return {float3::zeroes, 0.0F};
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
  using namespace eely::internal;

  bit_writer_write(writer, value.x);
  bit_writer_write(writer, value.y);
  bit_writer_write(writer, value.z);
  bit_writer_write(writer, value.w);
}
}  // namespace internal
}  // namespace eely