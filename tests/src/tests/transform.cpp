#include "tests/test_utils.h"

#include "eely/math/quaternion.h"
#include <eely/math/transform.h>

#include <gtest/gtest.h>

TEST(transform, constructor)
{
  using namespace eely;

  // Constructor
  {
    const float3 t{0.2F, -1.0F, 0.13F};
    const quaternion r{0.2F, 0.15F, 2.4F, 1.13F};
    const float3 s{2.0F, 5.2F, 11.0F};

    const transform transform{t, r, s};
    EXPECT_EQ(transform.translation, t);
    EXPECT_EQ(transform.rotation, r);
    EXPECT_EQ(transform.scale, s);
  }

  // Copy constructor
  {
    const float3 t{0.1F, 0.3F, -0.11F};
    const quaternion r{12.11F, 0.2F, -0.3F, 0.9F};
    const float3 s{0.5F, 0.25F, 0.85F};

    transform transform{};
    transform = eely::transform{t, r, s};
    EXPECT_EQ(transform.translation, t);
    EXPECT_EQ(transform.rotation, r);
    EXPECT_EQ(transform.scale, s);
  }
}

TEST(transform, operators)
{
  using namespace eely;

  static constexpr float epsilon = 1e-3F;

  // operator*(transform, transform)
  {
    const transform t0{float3{0.0F, 1.0F, 0.0F},
                       quaternion_from_axis_angle(1.0F, 0.0F, 0.0F, pi / 2.0F),
                       float3{2.0F, 1.0F, 2.0F}};
    const transform t1{float3{1.0F, -1.0F, 2.0F},
                       quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, pi / 2.0F),
                       float3{2.0F, 0.25F, 1.5F}};

    expect_transform_near(
        t0 * t1,
        transform{float3{2.0F, -3.0F, -1.0F}, t0.rotation * t1.rotation, {4.0F, 0.25F, 3.0F}},
        epsilon);

    const transform t2{float3{1.0F, 1.0F, 1.0F}};
    expect_transform_near(
        t0 * t1 * t2,
        transform{float3{5.0F, 1.0F, -0.75F}, t0.rotation * t1.rotation, t0.scale * t1.scale},
        epsilon);
  }

  // operator==(transform, transform)
  // operator!=(transform, transform)
  {
    const transform t0{float3{1.0F, 0.0F, 0.0F},
                       quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, 2.0F),
                       float3{1.0F, 3.0F, 1.0F}};
    transform t1{float3{1.0F, 0.0F, 0.05F},
                 quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, 2.0F),
                 float3{1.0F, 3.0F, 1.0F}};
    EXPECT_FALSE(t0 == t1);
    EXPECT_TRUE(t0 != t1);

    t1 = t0;
    EXPECT_TRUE(t0 == t1);
    EXPECT_FALSE(t0 != t1);
  }
}

TEST(transform, utils)
{
  using namespace eely;
  using namespace eely::internal;

  constexpr float epsilon{1E-5F};

  // transform_near(transform, transform, float)
  {
    const transform t0{float3{0.0F, 0.0F, 0.0F},
                       quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, 0.0F),
                       float3{1.0F, 1.0F, 1.0F}};
    EXPECT_TRUE(transform_near(t0, t0));

    transform t1{t0};
    t1.translation.x = t0.translation.x + epsilon * 0.5F;
    EXPECT_TRUE(transform_near(t0, t1));

    t1 = t0;
    t1.translation.y = t0.translation.x + epsilon * 1.05F;
    EXPECT_FALSE(transform_near(t0, t1));

    t1 = t0;
    t1.rotation.w = t0.rotation.w + epsilon * 0.8F;
    EXPECT_TRUE(transform_near(t0, t1));

    t1 = t0;
    t1.rotation.w = t0.rotation.w + epsilon * 1.01F;
    EXPECT_FALSE(transform_near(t0, t1));

    t1 = t0;
    t1.scale.y = t0.scale.y + epsilon * 0.98F;
    EXPECT_TRUE(transform_near(t0, t1));

    t1 = t0;
    t1.scale.y = t0.scale.y + epsilon * 1.005F;
    EXPECT_FALSE(transform_near(t0, t1));
  }

  // transform_serialize & transform_deserialize
  {
    const transform t{float3{0.173F, 0.654F, 0.175F}, quaternion{0.56F, 0.11F, 0.051F, 0.78F},
                      float3{9.4F, 0.13F, -6.78F}};

    std::array<std::byte, 40> buffer;
    bit_reader reader{buffer};
    bit_writer writer{buffer};

    bit_writer_write(writer, t);
    const transform t_deserialized{bit_reader_read<transform>(reader)};

    EXPECT_EQ(t, t_deserialized);
  }
}