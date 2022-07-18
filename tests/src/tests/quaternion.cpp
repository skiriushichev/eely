#include "tests/test_utils.h"

#include <eely/bit_reader.h>
#include <eely/bit_writer.h>
#include <eely/math_utils.h>
#include <eely/quaternion.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(quaternion, constructor)
{
  using namespace eely;

  // Constructor
  {
    const quaternion q{quaternion::identity};
    EXPECT_EQ(q.x, 0.0F);
    EXPECT_EQ(q.y, 0.0F);
    EXPECT_EQ(q.z, 0.0F);
    EXPECT_EQ(q.w, 1.0F);
  }

  // Copy constructor
  {
    quaternion q{};
    q = quaternion{0.19F, -0.11F, 2.01F, -9.31F};
    EXPECT_EQ(q.x, 0.19F);
    EXPECT_EQ(q.y, -0.11F);
    EXPECT_EQ(q.z, 2.01F);
    EXPECT_EQ(q.w, -9.31F);
  }
}

TEST(quaternion, operators)
{
  using namespace eely;

  constexpr float epsilon{1E-5F};

  // operator*
  {
    const quaternion q0{0.19F, -0.11F, 2.01F, -9.31F};
    const quaternion q1{3.9F, 0.121F, -4.75F, -0.683F};
    const quaternion q{q0 * q1};
    EXPECT_NEAR(q.x, -36.15948F, epsilon);
    EXPECT_NEAR(q.y, 7.69012F, epsilon);
    EXPECT_NEAR(q.z, 43.30166F, epsilon);
    EXPECT_NEAR(q.w, 15.17854F, epsilon);
  }

  // operator==
  // operator!=
  {
    const quaternion q0{0.19F, -0.11F, 2.01F, -9.31F};
    const quaternion q1{3.9F, 0.121F, -4.75F, -0.683F};
    EXPECT_TRUE(q0 == q0);
    EXPECT_TRUE(q1 == q1);
    EXPECT_TRUE(q0 != q1);
  }
}

TEST(quaternion, utils)
{
  using namespace eely;

  constexpr float epsilon{1e-3F};

  // quaternion_length + quaternion_normalize
  {
    quaternion q{0.4400896F, 0.3600733, 0.3920798F, 0.7231472F};
    EXPECT_NEAR(quaternion_length(q), 1.0F, epsilon);
    expect_quaternion_near(q, quaternion_normalized(q), epsilon);

    q = quaternion{2.0F, 3.0F, 19.0F, 5.45F};
    EXPECT_NEAR(quaternion_length(q), 20.0923F, epsilon);

    q = quaternion_normalized(q);
    EXPECT_NEAR(quaternion_length(q), 1.0F, epsilon);
  }

  // quaternion_inverse
  {
    const quaternion q{0.4400896F, 0.3600733, 0.3920798F, 0.7231472F};
    const quaternion q_inversed{quaternion_inverse(q)};
    EXPECT_NEAR(q_inversed.x, -q.x, epsilon);
    EXPECT_NEAR(q_inversed.y, -q.y, epsilon);
    EXPECT_NEAR(q_inversed.z, -q.z, epsilon);
    EXPECT_NEAR(q_inversed.w, q.w, epsilon);
  }

  // quaternion_from_yaw_pitch_roll_intrinsic
  {
    quaternion q{quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, 0.0F)};
    EXPECT_NEAR(q.x, 0.0F, epsilon);
    EXPECT_NEAR(q.y, 0.0F, epsilon);
    EXPECT_NEAR(q.z, 0.0F, epsilon);
    EXPECT_NEAR(q.w, 1.0F, epsilon);

    q = quaternion_from_yaw_pitch_roll_intrinsic(0.524F, 0.262F, -1.309F);
    EXPECT_NEAR(q.x, -0.0562307F, epsilon);
    EXPECT_NEAR(q.y, 0.2805341F, epsilon);
    EXPECT_NEAR(q.z, -0.609792F, epsilon);
    EXPECT_NEAR(q.w, 0.739116F, epsilon);

    q = quaternion_from_yaw_pitch_roll_intrinsic(-3.1F, 0.717F, -2.95F);
    EXPECT_NEAR(q.x, 0.932627F, epsilon);
    EXPECT_NEAR(q.y, -0.0822867F, epsilon);
    EXPECT_NEAR(q.z, 0.0141699F, epsilon);
    EXPECT_NEAR(q.w, 0.3510483F, epsilon);
  }

  // quaternion_from_axis_angle
  {
    quaternion q{quaternion_from_axis_angle(1.0F, 3.0F, 5.0F, 0.65F)};
    expect_quaternion_near(q, quaternion{0.053973F, 0.1619191F, 0.2698652F, 0.9476507F}, epsilon);

    q = quaternion_from_axis_angle(0.717F, 0.717F, 0.0F, -3.19F);
    expect_quaternion_near(q, quaternion{-0.7068997F, -0.7068997F, 0.0F, -0.0242013F}, epsilon);

    q = quaternion_from_axis_angle(-1.2F, 3.0F, 15.0F, 0.07F);
    expect_quaternion_near(q, quaternion{-0.0027367F, 0.0068416F, 0.0342082F, 0.9993876F}, epsilon);
  }

  // quaternion_slerp & quaternion_from_axis_angle & quaternion_to_axis_angle
  {
    // Quaternions slerp should lead to angle being interpolated linearly, check for that

    auto slerp_test = [](const float3& axis, const float angle_from, const float angle_to) {
      quaternion q0{quaternion_from_axis_angle(axis.x, axis.y, axis.z, angle_from)};
      quaternion q1{quaternion_from_axis_angle(axis.x, axis.y, axis.z, angle_to)};
      expect_quaternion_near(quaternion_slerp(q0, q1, 0.0F), q0, epsilon);
      expect_quaternion_near(quaternion_slerp(q0, q1, 1.0F), q1, epsilon);

      quaternion q_slerped{quaternion_slerp(q0, q1, 0.1F)};
      std::pair<float3, float> axis_angle{quaternion_to_axis_angle(q_slerped)};
      expect_float3_near(axis_angle.first, axis, epsilon);
      EXPECT_NEAR(axis_angle.second, std::lerp(angle_from, angle_to, 0.1F), epsilon);

      q_slerped = quaternion_slerp(q0, q1, 0.4F);
      axis_angle = quaternion_to_axis_angle(q_slerped);
      expect_float3_near(axis_angle.first, axis, epsilon);
      EXPECT_NEAR(axis_angle.second, std::lerp(angle_from, angle_to, 0.4F), epsilon);

      q_slerped = quaternion_slerp(q0, q1, 0.5F);
      axis_angle = quaternion_to_axis_angle(q_slerped);
      expect_float3_near(axis_angle.first, axis, epsilon);
      EXPECT_NEAR(axis_angle.second, std::lerp(angle_from, angle_to, 0.5F), epsilon);

      q_slerped = quaternion_slerp(q0, q1, 0.7F);
      axis_angle = quaternion_to_axis_angle(q_slerped);
      expect_float3_near(axis_angle.first, axis, epsilon);
      EXPECT_NEAR(axis_angle.second, std::lerp(angle_from, angle_to, 0.7F), epsilon);

      q_slerped = quaternion_slerp(q0, q1, 0.99F);
      axis_angle = quaternion_to_axis_angle(q_slerped);
      expect_float3_near(axis_angle.first, axis, epsilon);
      EXPECT_NEAR(axis_angle.second, std::lerp(angle_from, angle_to, 0.99F), epsilon);
    };

    slerp_test(float3{0.0F, 1.0F, 0.0F}, 2.0F, 3.0F);
    slerp_test(vector_normalized(float3{1.0F, 1.0F, 0.0F}), 1.15F, 1.23F);
    slerp_test(vector_normalized(float3{1.0F, 1.0F, 1.0F}), 3.0F, 3.11F);
  }

  // quaternion_near
  {
    const quaternion q{0.4400896F, 0.3600733, 0.3920798F, 0.7231472F};
    EXPECT_TRUE(quaternion_near(q, q));

    const quaternion q0_plus_half_epsilon{q.x + epsilon / 2.0F, q.y + epsilon / 2.0F,
                                          q.z + epsilon / 2.0F, q.w + epsilon / 2.0F};
    EXPECT_TRUE(quaternion_near(q, q0_plus_half_epsilon, epsilon));

    const quaternion q0_minus_half_epsilon{q.x - epsilon / 2.0F, q.y - epsilon / 2.0F,
                                           q.z - epsilon / 2.0F, q.w - epsilon / 2.0F};
    EXPECT_TRUE(quaternion_near(q, q0_minus_half_epsilon, epsilon));

    const quaternion q0_with_x_slightly_bigger_than_epsilon{q.x + epsilon * 1.2F, q.y, q.z, q.w};
    EXPECT_FALSE(quaternion_near(q, q0_with_x_slightly_bigger_than_epsilon, epsilon));
  }

  // quaternion_serialize & quaternion_deserialize
  {
    const quaternion q{0.173F, 0.654F, 0.175F, 0.715F};

    std::array<std::byte, 16> buffer;
    bit_reader reader{buffer};
    bit_writer writer{buffer};

    quaternion_serialize(q, writer);
    const quaternion q_deserialized{quaternion_deserialize(reader)};

    EXPECT_EQ(q, q_deserialized);
  }

  // quaternion_from_yaw_pitch_roll_intrinsic(float, float, float)
  // vector_rotate(float3, quaternion)
  {
    quaternion rotation{quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, 0.0F)};
    float3 v{1.0F, 1.0F, 1.0F};
    float3 v_rotated{vector_rotate(v, rotation)};
    EXPECT_NEAR(v_rotated.x, 1.0F, epsilon);
    EXPECT_NEAR(v_rotated.y, 1.0F, epsilon);
    EXPECT_NEAR(v_rotated.z, 1.0F, epsilon);

    rotation = quaternion_from_yaw_pitch_roll_intrinsic(0.0F, 0.0F, pi);
    v = float3{1.0F, 0.0F, 0.0F};
    v_rotated = vector_rotate(v, rotation);
    EXPECT_NEAR(v_rotated.x, -1.0F, epsilon);
    EXPECT_NEAR(v_rotated.y, 0.0F, epsilon);
    EXPECT_NEAR(v_rotated.z, 0.0F, epsilon);

    rotation = quaternion_from_yaw_pitch_roll_intrinsic(pi / 2.0F, 0.0F, pi / 2.0F);
    v = float3{1.0F, 0.0F, 0.0F};
    v_rotated = vector_rotate(v, rotation);
    EXPECT_NEAR(v_rotated.x, 0.0F, epsilon);
    EXPECT_NEAR(v_rotated.y, 1.0F, epsilon);
    EXPECT_NEAR(v_rotated.z, 0.0F, epsilon);

    rotation = quaternion_from_yaw_pitch_roll_intrinsic(pi, pi / 4.0F, pi / 3.0F);
    v = float3{1.0F, 1.0F, 1.0F};
    v_rotated = vector_rotate(v, rotation);
    EXPECT_NEAR(v_rotated.x, 0.36603F, epsilon);
    EXPECT_NEAR(v_rotated.y, 0.25882F, epsilon);
    EXPECT_NEAR(v_rotated.z, -1.67303F, epsilon);

    rotation = quaternion_from_yaw_pitch_roll_intrinsic(-3.1F, 0.28F, 0.37F);
    v = float3{-2.0F, 0.13F, 14.0F};
    v_rotated = vector_rotate(v, rotation);
    EXPECT_NEAR(v_rotated.x, 1.35747F, epsilon);
    EXPECT_NEAR(v_rotated.y, -4.44756F, epsilon);
    EXPECT_NEAR(v_rotated.z, -13.3564F, epsilon);
  }
}
