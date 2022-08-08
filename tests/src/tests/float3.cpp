#include "tests/test_utils.h"

#include <eely/base/bit_reader.h>
#include <eely/base/bit_writer.h>
#include <eely/math/float3.h>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

TEST(float3, constructors)
{
  using namespace eely;

  // Constructor
  {
    const float3 v{0.0F, 0.0F, 0.0F};
    EXPECT_EQ(v.x, 0.0F);
    EXPECT_EQ(v.y, 0.0F);
    EXPECT_EQ(v.z, 0.0F);
  }

  // Copy constructor
  {
    float3 v{};
    v = float3{3.3F, -12.10F, -0.33F};
    EXPECT_EQ(v.x, 3.3F);
    EXPECT_EQ(v.y, -12.10F);
    EXPECT_EQ(v.z, -0.330F);
  }
}

TEST(float3, operators)
{
  using namespace eely;

  constexpr float epsilon{1E-5F};

  // operator+(float3, float3)
  {
    const float3 v{float3{3.2F, 0.15F, -11.39F} + float3{0.19F, 2.2F, -7.19F}};
    EXPECT_NEAR(v.x, 3.39F, epsilon);
    EXPECT_NEAR(v.y, 2.35F, epsilon);
    EXPECT_NEAR(v.z, -18.58F, epsilon);
  }

  // operator-(float3, float3)
  {
    const float3 v{float3{3.2F, 0.15F, -11.39F} - float3{0.19F, 2.2F, -7.19F}};
    EXPECT_NEAR(v.x, 3.01F, epsilon);
    EXPECT_NEAR(v.y, -2.05F, epsilon);
    EXPECT_NEAR(v.z, -4.2F, epsilon);
  }

  // operator-(float3)
  {
    const float3 v{-float3{3.2F, 0.15F, -11.39F}};
    EXPECT_NEAR(v.x, -3.2F, epsilon);
    EXPECT_NEAR(v.y, -0.15F, epsilon);
    EXPECT_NEAR(v.z, 11.39F, epsilon);
  }

  // operator*(float3, float)
  {
    const float3 v{float3{3.2F, 0.15F, -11.39F} * 0.24F};
    EXPECT_NEAR(v.x, 0.768F, epsilon);
    EXPECT_NEAR(v.y, 0.036F, epsilon);
    EXPECT_NEAR(v.z, -2.7336F, epsilon);
  }

  // operator*(float, float3)
  {
    const float3 v{0.24F * float3{3.2F, 0.15F, -11.39F}};
    EXPECT_NEAR(v.x, 0.768F, epsilon);
    EXPECT_NEAR(v.y, 0.036F, epsilon);
    EXPECT_NEAR(v.z, -2.7336F, epsilon);
  }

  // operator*(float3, float3)
  {
    const float3 v = float3{3.2F, 0.15F, -11.39F} * float3{0.19F, 2.2F, -7.19F};
    EXPECT_NEAR(v.x, 0.608F, epsilon);
    EXPECT_NEAR(v.y, 0.33F, epsilon);
    EXPECT_NEAR(v.z, 81.8941F, epsilon);
  }

  // operator==(float3, float3)
  // operator!=(float3, float3)
  {
    const float3 a{3.2F, 0.15F, -11.39F};
    const float3 b{0.19F, 2.2F, -7.19F};
    EXPECT_TRUE(a == a);
    EXPECT_TRUE(a != b);
  }
}

TEST(float3, utils)
{
  using namespace eely;

  constexpr float epsilon{1E-5F};

  // float3_lerp
  {
    const float3 a{1.0F, 2.0F, 5.0F};
    const float3 b{6.0F, 1.0F, 0.0F};

    expect_float3_near(float3_lerp(a, b, 0.0F), a, epsilon);
    expect_float3_near(float3_lerp(a, b, 1.0F), b, epsilon);
    expect_float3_near(float3_lerp(a, b, 0.5F), float3{3.5F, 1.5F, 2.5F});
    expect_float3_near(float3_lerp(a, b, 0.2F), float3{2.0F, 1.8F, 4.0F});
  }

  // float3_near
  {
    const float3 v{3.2F, 0.15F, -11.39F};

    expect_float3_near(v, v);

    expect_float3_near(v, v + float3{epsilon / 2.0F, epsilon / 2.0F, epsilon / 2.0F}, epsilon);
    expect_float3_near(v, v - float3{epsilon / 2.0F, epsilon / 2.0F, epsilon / 2.0F}, epsilon);
    expect_float3_not_near(v, v + float3{epsilon * 1.2F, 0.0F, 0.0F}, epsilon);
  }

  // float3_serialize & float3_deserialize
  {
    const float3 v{0.19F, 2.2F, -7.19F};

    std::array<std::byte, 12> buffer;
    bit_reader reader{buffer};
    bit_writer writer{buffer};

    float3_serialize(v, writer);
    const float3 v_deserialized{float3_deserialize(reader)};

    EXPECT_EQ(v, v_deserialized);
  }

  // vector_normalized
  {
    const float3 v{3.2F, 0.15F, -11.39F};
    const float3 v_normalized{vector_normalized(v)};

    expect_float3_near(v_normalized, float3{0.27045F, 0.01267F, -0.96264F}, epsilon);
  }

  // vector_dot
  {
    const float3 a{3.2F, 0.15F, -11.39F};
    const float3 b{0.19F, 2.2F, -7.19F};

    const float dot{vector_dot(float3{3.2F, 0.15F, -11.39F}, float3{0.19F, 2.2F, -7.19F})};
    EXPECT_NEAR(dot, 82.8321F, epsilon);
  }

  // vector_cross
  {
    const float3 c{vector_cross(float3{3.2F, 0.15F, -11.39F}, float3{0.19F, 2.2F, -7.19F})};
    EXPECT_NEAR(c.x, 23.9795F, epsilon);
    EXPECT_NEAR(c.y, 20.8439F, epsilon);
    EXPECT_NEAR(c.z, 7.0115F, epsilon);
  }
}