#include "eely/float3.h"

#include "eely/base_utils.h"
#include "eely/math_utils.h"

#include <gsl/assert>

#include <cmath>

namespace eely {
const float3 float3::zeroes{0.0F, 0.0F, 0.0F};
const float3 float3::ones{1.0F, 1.0F, 1.0F};

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

void float3_serialize(const float3& a, bit_writer& writer)
{
  writer.write({.value = bit_cast<uint32_t>(a.x), .size_bits = 32});
  writer.write({.value = bit_cast<uint32_t>(a.y), .size_bits = 32});
  writer.write({.value = bit_cast<uint32_t>(a.z), .size_bits = 32});
}

float3 float3_deserialize(bit_reader& reader)
{
  return float3{.x = bit_cast<float>(reader.read(32)),
                .y = bit_cast<float>(reader.read(32)),
                .z = bit_cast<float>(reader.read(32))};
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
}  // namespace eely