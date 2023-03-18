#include "eely/math/float3.h"

#include "eely/base/base_utils.h"
#include "eely/math/math_utils.h"

#include <cmath>

namespace eely {
const float3 float3::zeroes{0.0F, 0.0F, 0.0F};
const float3 float3::ones{1.0F, 1.0F, 1.0F};
const float3 float3::x_axis{1.0F, 0.0F, 0.0F};
const float3 float3::y_axis{0.0F, 1.0F, 0.0F};
const float3 float3::z_axis{0.0F, 0.0F, 1.0F};

float3 operator+(const float3& a, const float3& b)
{
  return float3{a.x + b.x, a.y + b.y, a.z + b.z};
}

float3& operator+=(float3& a, const float3& b)
{
  a = a + b;
  return a;
}

float3 operator-(const float3& a, const float3& b)
{
  return float3{a.x - b.x, a.y - b.y, a.z - b.z};
}

float3& operator-=(float3& a, const float3& b)
{
  a = a - b;
  return a;
}

float3 operator-(const float3& a)
{
  return float3{-a.x, -a.y, -a.z};
}

float3 operator*(const float3& a, const float scalar)
{
  return float3{a.x * scalar, a.y * scalar, a.z * scalar};
}

float3 operator*(const float scalar, const float3& a)
{
  return a * scalar;
}

float3 operator*(const float3& a, const float3& b)
{
  return float3{a.x * b.x, a.y * b.y, a.z * b.z};
}

bool operator==(const float3& a, const float3& b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!=(const float3& a, const float3& b)
{
  return !(a == b);
}

float3 float3_lerp(const float3& a, const float3& b, float t)
{
  return float3{std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t)};
}

bool float3_near(const float3& a, const float3& b, const float epsilon)
{
  return float_near(a.x, b.x, epsilon) && float_near(a.y, b.y, epsilon) &&
         float_near(a.z, b.z, epsilon);
}

float float3_distance(const float3& a, const float3& b)
{
  return vector_length(a - b);
}

float3 vector_normalized(const float3& a)
{
  const float length{std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z)};
  return float3{a.x / length, a.y / length, a.z / length};
}

float vector_dot(const float3& a, const float3& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 vector_cross(const float3& a, const float3& b)
{
  return float3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float vector_length(const float3& a)
{
  return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

namespace internal {
void bit_writer_write(bit_writer& writer, const float3& value)
{
  writer.write({.value = bit_cast<uint32_t>(value.x), .size_bits = 32});
  writer.write({.value = bit_cast<uint32_t>(value.y), .size_bits = 32});
  writer.write({.value = bit_cast<uint32_t>(value.z), .size_bits = 32});
}
}  // namespace internal
}  // namespace eely