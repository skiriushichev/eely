#include <eely_app/matrix4x4.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(matrix4x4, constructor)
{
  using namespace eely;

  // Constructor
  {
    // clang-format off
        const matrix4x4 m = matrix4x4{
            1.0F, 2.0F, 3.0F, 4.0F,
            5.0F, 6.0F, 7.0F, 8.0F,
            9.0F, 10.0F, 11.0F, 12.0F,
            13.0F, 14.0F, 15.0F, 16.0F};
    // clang-format on

    EXPECT_EQ(m(0, 0), 1.0F);
    EXPECT_EQ(m(0, 1), 2.0F);
    EXPECT_EQ(m(0, 2), 3.0F);
    EXPECT_EQ(m(0, 3), 4.0F);

    EXPECT_EQ(m(1, 0), 5.0F);
    EXPECT_EQ(m(1, 1), 6.0F);
    EXPECT_EQ(m(1, 2), 7.0F);
    EXPECT_EQ(m(1, 3), 8.0F);

    EXPECT_EQ(m(2, 0), 9.0F);
    EXPECT_EQ(m(2, 1), 10.0F);
    EXPECT_EQ(m(2, 2), 11.0F);
    EXPECT_EQ(m(2, 3), 12.0F);

    EXPECT_EQ(m(3, 0), 13.0F);
    EXPECT_EQ(m(3, 1), 14.0F);
    EXPECT_EQ(m(3, 2), 15.0F);
    EXPECT_EQ(m(3, 3), 16.0F);
  }

  // Copy constructor
  {
    matrix4x4 m{};

    // clang-format off
        m = matrix4x4{
            17.0F, 18.0F, 19.0F, 20.0F,
            21.0F, 22.0F, 23.0F, 24.0F,
            25.0F, 26.0F, 27.0F, 28.0F,
            29.0F, 30.0F, 31.0F, 32.0F};
    // clang-format on

    EXPECT_EQ(m(0, 0), 17.0F);
    EXPECT_EQ(m(0, 1), 18.0F);
    EXPECT_EQ(m(0, 2), 19.0F);
    EXPECT_EQ(m(0, 3), 20.0F);

    EXPECT_EQ(m(1, 0), 21.0F);
    EXPECT_EQ(m(1, 1), 22.0F);
    EXPECT_EQ(m(1, 2), 23.0F);
    EXPECT_EQ(m(1, 3), 24.0F);

    EXPECT_EQ(m(2, 0), 25.0F);
    EXPECT_EQ(m(2, 1), 26.0F);
    EXPECT_EQ(m(2, 2), 27.0F);
    EXPECT_EQ(m(2, 3), 28.0F);

    EXPECT_EQ(m(3, 0), 29.0F);
    EXPECT_EQ(m(3, 1), 30.0F);
    EXPECT_EQ(m(3, 2), 31.0F);
    EXPECT_EQ(m(3, 3), 32.0F);
  }
}

TEST(matrix4x4, operators)
{
  using namespace eely;

  constexpr float epsilon{1E-3F};

  // operator*
  {
    // clang-format off
        const matrix4x4 m0{
            4.0F, 7.0F, 5.0F, 11.0F,
            3.0F, 2.0F, -7.0F, -9.0F,
            -2.0F, 15.7F, 21.0F, 0.1F,
            17.0F, -1.0F, 5.0F, 13.3F};
    // clang-format on

    // clang-format off
        const matrix4x4 m1{
            0.19F, 22.0F, -3.3F, 0.1F,
            0.22F, -159.0F, -2.0F, 4.5F,
            5.87F, 3.2F, -20.19F, 0.23F,
            -15.05F, 22.19F, -22.11F, 6.6F};
    // clang-format on

    const matrix4x4 m = m0 * m1;

    EXPECT_NEAR(m(0, 0), -133.9F, epsilon);
    EXPECT_NEAR(m(0, 1), -764.91F, epsilon);
    EXPECT_NEAR(m(0, 2), -371.36F, epsilon);
    EXPECT_NEAR(m(0, 3), 105.65F, epsilon);

    EXPECT_NEAR(m(1, 0), 95.37F, epsilon);
    EXPECT_NEAR(m(1, 1), -474.11F, epsilon);
    EXPECT_NEAR(m(1, 2), 326.42F, epsilon);
    EXPECT_NEAR(m(1, 3), -51.71F, epsilon);

    EXPECT_NEAR(m(2, 0), 124.839F, epsilon);
    EXPECT_NEAR(m(2, 1), -2470.881F, epsilon);
    EXPECT_NEAR(m(2, 2), -451.001F, epsilon);
    EXPECT_NEAR(m(2, 3), 75.94F, epsilon);

    EXPECT_NEAR(m(3, 0), -167.805F, epsilon);
    EXPECT_NEAR(m(3, 1), 844.127F, epsilon);
    EXPECT_NEAR(m(3, 2), -449.113F, epsilon);
    EXPECT_NEAR(m(3, 3), 86.13F, epsilon);
  }

  // operator== & operator!=
  {
    const matrix4x4 m0{matrix4x4::identity};

    matrix4x4 m1{m0};
    m1(0, 3) = 12.0F;

    EXPECT_EQ(m0, m0);
    EXPECT_EQ(m1, m1);
    EXPECT_NE(m0, m1);
    EXPECT_NE(m1, m0);
  }
}

TEST(matrix4x4, utils)
{
  using namespace eely;

  static constexpr float epsilon{1E-4F};

  // matrix4x4_near
  {
    // clang-format off
        const matrix4x4 m0{
            4.0F, 7.0F, 5.0F, 11.0F,
            3.0F, 2.0F, -7.0F, -9.0F,
            -2.0F, 15.7F, 21.0F, 0.1F,
            17.0F, -1.0F, 5.0F, 13.3F};
    // clang-format on

    EXPECT_TRUE(matrix4x4_near(m0, m0));

    matrix4x4 m1{m0};

    m1(2, 2) += epsilon * 0.8F;
    EXPECT_TRUE(matrix4x4_near(m0, m1, epsilon));

    m1(3, 1) += epsilon * 1.1F;
    EXPECT_FALSE(matrix4x4_near(m0, m1, epsilon));
  }

  // matrix4x4_from_translation
  // matrix4x4_transform_location
  // matrix4x4_transform_direction
  {
    const matrix4x4 m{matrix4x4_from_translation(0.2F, 1.0F, -2.0F)};

    float3 location{0.12F, 11.0F, 0.2F};
    location = matrix4x4_transform_location(m, location);
    EXPECT_NEAR(location.x, 0.32F, epsilon);
    EXPECT_NEAR(location.y, 12.0F, epsilon);
    EXPECT_NEAR(location.z, -1.8F, epsilon);

    float3 direction{0.707F, 0.0F, 0.707F};
    direction = matrix4x4_transform_direction(m, direction);
    EXPECT_EQ(direction.x, 0.707F);
    EXPECT_EQ(direction.y, 0.0F);
    EXPECT_EQ(direction.z, 0.707F);
  }

  // matrix4x4_from_scale
  // matrix4x4_transform_location
  // matrix4x4_transform_direction
  {
    const matrix4x4 m{matrix4x4_from_scale(0.5F, 0.8F, 0.1F)};

    float3 location{0.12F, 11.0F, 0.2F};
    location = matrix4x4_transform_location(m, location);
    EXPECT_NEAR(location.x, 0.06F, epsilon);
    EXPECT_NEAR(location.y, 8.8F, epsilon);
    EXPECT_NEAR(location.z, 0.02F, epsilon);

    float3 direction{0.707F, 0.0F, 0.707F};
    direction = matrix4x4_transform_direction(m, direction);
    EXPECT_NEAR(direction.x, 0.3535F, epsilon);
    EXPECT_NEAR(direction.y, 0.0F, epsilon);
    EXPECT_NEAR(direction.z, 0.0707F, epsilon);
  }

  // matrix4x4_from_rotation
  // matrix4x4_transform_location
  // matrix4x4_transform_direction
  {
    const matrix4x4 m{matrix4x4_from_rotation(
        quaternion_from_yaw_pitch_roll_intrinsic(pi, pi / 4.0F, pi / 3.0F))};

    float3 location{0.5F, -1.0F, 2.0F};
    location = matrix4x4_transform_location(m, location);
    EXPECT_NEAR(location.x, -1.11603F, epsilon);
    EXPECT_NEAR(location.y, -1.46158F, epsilon);
    EXPECT_NEAR(location.z, -1.36685F, epsilon);

    float3 direction{0.707F, 0.0F, 0.707F};
    direction = matrix4x4_transform_direction(m, direction);
    EXPECT_NEAR(direction.x, -0.3535F, epsilon);
    EXPECT_NEAR(direction.y, -0.06698F, epsilon);
    EXPECT_NEAR(direction.z, -0.93287F, epsilon);
  }

  // matrix4x4_from_transform
  // matrix4x4_transform_location
  // matrix4x4_transform_direction
  {
    const matrix4x4 m0{matrix4x4_from_transform(
        {float3{1.0F, -1.0F, 0.0F},
         quaternion_from_yaw_pitch_roll_intrinsic(0.0F, pi / 2.0F, pi / 2.0F),
         float3{2.0F, 0.5F, 0.25F}})};

    float3 location{0.5F, 0.2F, -1.0F};
    location = matrix4x4_transform_location(m0, location);
    EXPECT_NEAR(location.x, 0.9F, epsilon);
    EXPECT_NEAR(location.y, -0.75F, epsilon);
    EXPECT_NEAR(location.z, 1.0F, epsilon);

    float3 direction{1.0F, 0.0F, 0.0F};
    direction = matrix4x4_transform_direction(m0, direction);
    EXPECT_NEAR(direction.x, 0.0F, epsilon);
    EXPECT_NEAR(direction.y, 0.0F, epsilon);
    EXPECT_NEAR(direction.z, 2.0F, epsilon);

    direction = float3{0.0F, 1.0F, 0.0F};
    direction = matrix4x4_transform_direction(m0, direction);
    EXPECT_NEAR(direction.x, -0.5F, epsilon);
    EXPECT_NEAR(direction.y, 0.0F, epsilon);
    EXPECT_NEAR(direction.z, 0.0F, epsilon);

    direction = float3{0.0F, 0.0F, 1.0F};
    direction = matrix4x4_transform_direction(m0, direction);
    EXPECT_NEAR(direction.x, 0.0F, epsilon);
    EXPECT_NEAR(direction.y, -0.25F, epsilon);
    EXPECT_NEAR(direction.z, 0.0F, epsilon);
  }

  // matrix4x4_clip_space
  // matrix4x4_transform
  {
    clip_space_params params{.fov_x = pi / 2.0F,
                             .near = 0.01F,
                             .far = 100.0F,
                             .aspect_ratio_x_to_y = 100.0F / 200.0F,
                             .depth_range = clip_space_depth_range::minus_one_to_plus_one};

    const float projection_source_plane_z{10.0F};
    const float projection_source_plane_length_x{2.0F * projection_source_plane_z *
                                                 std::tan(params.fov_x / 2.0F)};
    const float projection_source_plane_length_y{projection_source_plane_length_x /
                                                 params.aspect_ratio_x_to_y};

    matrix4x4 matrix_clip{matrix4x4_clip_space(params)};

    // Near to -w
    float4 v{0.0F, 0.0F, params.near, 1.0F};
    float4 v_clip_space{matrix4x4_transform(matrix_clip, v)};
    EXPECT_NEAR(v_clip_space.z, -v_clip_space.w, epsilon);

    // Far to w
    v = float4{0.0F, 0.0F, params.far, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.z, v_clip_space.w, epsilon);

    params.depth_range = clip_space_depth_range::zero_to_plus_one;
    matrix_clip = matrix4x4_clip_space(params);

    // Near to 0
    v = float4{0.0F, 0.0F, params.near, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.z, 0.0F, epsilon);

    // Far to w
    v = float4{0.0F, 0.0F, params.far, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.z, v_clip_space.w, epsilon);

    // Left source plane point to -w
    v = float4{-projection_source_plane_length_x / 2.0F, 0.0F, projection_source_plane_z, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.x, -v_clip_space.w, epsilon);

    // Right source plane point to w
    v = float4{projection_source_plane_length_x / 2.0F, 0.0F, projection_source_plane_z, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.x, v_clip_space.w, epsilon);

    // Bottom source plane point to -w
    v = float4{0.0F, -projection_source_plane_length_y / 2.0F, projection_source_plane_z, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.y, -v_clip_space.w, epsilon);

    // Top source plane point to w
    v = float4{0.0F, projection_source_plane_length_y / 2.0F, projection_source_plane_z, 1.0F};
    v_clip_space = matrix4x4_transform(matrix_clip, v);
    EXPECT_NEAR(v_clip_space.y, v_clip_space.w, epsilon);
  }

  // matrix4x4_inverse
  {
    matrix4x4 m{matrix4x4::identity};
    matrix4x4 m_inversed{matrix4x4_inverse(m)};
    EXPECT_TRUE(matrix4x4_near(m, m_inversed, epsilon));

    // clang-format off
        m = matrix4x4{
            1.2F,   -3.0F,  3.31F, 9.7F,
            0.25F,  -0.3F,  1.2F,  2.05F,
            -2.11F, -3.87F, 0.5F,  0.75F,
            1.7F,   0.87F,  -3.0F, 12.0F };
        matrix4x4 m_inversed_expected{
            0.797232F, -2.48173F, -0.46860F, -0.191179F,
            -0.46843F, 1.565412F, 0.008264F, 0.110707F,
            -0.103899F, 0.93546F, -0.008894F, -0.075266F,
            -0.1049549F, 0.471951F, 0.06356F, 0.0835741F};
    // clang-format on
    m_inversed = matrix4x4_inverse(m);
    EXPECT_TRUE(matrix4x4_near(m_inversed, m_inversed_expected, epsilon));
  }
}