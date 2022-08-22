#include "tests/test_utils.h"

#include "eely/math/math_utils.h"
#include <eely/base/bit_reader.h>
#include <eely/base/bit_writer.h>
#include <eely/clip/clip.h>
#include <eely/clip/clip_player_base.h>
#include <eely/clip/clip_uncooked.h>
#include <eely/math/quaternion.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_pose.h>
#include <eely/skeleton/skeleton_uncooked.h>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <random>
#include <variant>
#include <vector>

static void test_skeleton_and_cip_with_scheme(eely::clip_compression_scheme compression_scheme)
{
  using namespace eely;

  std::array<std::byte, 1024> buffer;
  bit_reader reader{buffer};

  constexpr gsl::index root_index{0};
  constexpr gsl::index child_0_index{1};
  constexpr gsl::index child_1_index{2};

  const float3 root_translation_0{0.0F, -5.0F, 0.0F};
  const float3 child_0_translation_0{0.12F, 2.15F, 0.95F};
  const float3 child_0_translation_1{3.6F, -0.15F, 3.0F};
  const float3 child_0_translation_5{9.61F, -3.24F, 8.3F};
  const float3 child_0_translation_7{12.12F, 8.95F, 5.0F};
  const float3 child_0_translation_8{4.0F, 0.1F, 1.0F};
  const auto child_0_rotation_0{quaternion_from_yaw_pitch_roll_intrinsic(0.5F, -0.1F, 2.7F)};
  const auto child_0_rotation_3{quaternion_from_yaw_pitch_roll_intrinsic(0.1F, 1.7F, 0.05F)};
  const auto child_0_rotation_4{quaternion_from_yaw_pitch_roll_intrinsic(2.1F, 3.7F, 1.67F)};
  const auto child_0_rotation_7{quaternion_from_yaw_pitch_roll_intrinsic(0.66F, 3.12F, 1.98F)};
  const float3 child_0_scale_1{2.0F, 0.33F, 1.5F};
  const float3 child_0_scale_2{0.2F, 0.4F, 0.13F};
  const float3 child_0_scale_4{1.2F, 6.2F, 3.25F};
  const auto child_1_rotation_2{quaternion_from_yaw_pitch_roll_intrinsic(-0.6F, 0.32F, 1.97F)};

  // Cook test data
  {
    project_uncooked project_uncooked(measurement_unit::meters,
                                      axis_system::y_up_x_right_z_forward);

    auto skeleton_uncooked = std::make_unique<eely::skeleton_uncooked>("test_skeleton");
    skeleton_uncooked->set_joints(
        {{.id = "root", .parent_index = std::nullopt, .rest_pose_transform = transform{}},
         {.id = "child_0",
          .parent_index = 0,
          .rest_pose_transform = transform{float3{-1.0F, 0.0F, 0.0F}}},
         {.id = "child_1",
          .parent_index = 0,
          .rest_pose_transform = transform{float3{1.0F, 0.0F, 0.0F}}}});

    EXPECT_EQ(skeleton_uncooked->get_id(), "test_skeleton");

    auto clip_uncooked = std::make_unique<eely::clip_uncooked>("test_clip");
    clip_uncooked->set_compression_scheme(compression_scheme);
    clip_uncooked->set_target_skeleton_id(skeleton_uncooked->get_id());
    clip_uncooked->set_tracks({
        {.joint_id = "root", .keys = {{0.0F, {.translation = root_translation_0}}}},
        {.joint_id = "child_0",
         .keys = {{0.0F, {.translation = child_0_translation_0, .rotation = child_0_rotation_0}},
                  {1.0F, {.translation = child_0_translation_1, .scale = child_0_scale_1}},
                  {2.0F, {.scale = child_0_scale_2}},
                  {3.0F, {.rotation = child_0_rotation_3}},
                  {4.0F, {.rotation = child_0_rotation_4, .scale = child_0_scale_4}},
                  {5.0F, {.translation = child_0_translation_5}},
                  {7.0F, {.translation = child_0_translation_7, .rotation = child_0_rotation_7}},
                  {8.0F, {.translation = child_0_translation_8}}}},
        {.joint_id = "child_1", .keys = {{2.0F, {.rotation = child_1_rotation_2}}}},
    });

    EXPECT_EQ(clip_uncooked->get_id(), "test_clip");
    EXPECT_EQ(clip_uncooked->get_target_skeleton_id(), skeleton_uncooked->get_id());
    EXPECT_EQ(clip_uncooked->get_duration_s(), 8.0F);

    project_uncooked.set_resource(std::move(skeleton_uncooked));
    project_uncooked.set_resource(std::move(clip_uncooked));

    bit_writer writer{buffer};

    project::cook(project_uncooked, writer);
  }

  project project{reader};

  const skeleton* skeleton{project.get_resource<eely::skeleton>("test_skeleton")};
  EXPECT_EQ(skeleton->get_joints_count(), 3);
  EXPECT_EQ(skeleton->get_joint_index("root"), root_index);
  EXPECT_EQ(skeleton->get_joint_index("child_0"), child_0_index);
  EXPECT_EQ(skeleton->get_joint_index("child_1"), child_1_index);
  EXPECT_EQ(skeleton->get_joint_id(root_index), "root");
  EXPECT_EQ(skeleton->get_joint_id(child_0_index), "child_0");
  EXPECT_EQ(skeleton->get_joint_id(child_1_index), "child_1");
  EXPECT_EQ(skeleton->get_joint_parent_index(root_index), std::nullopt);
  EXPECT_EQ(skeleton->get_joint_parent_index(child_0_index), 0);
  EXPECT_EQ(skeleton->get_joint_parent_index(child_1_index), 0);
  EXPECT_EQ(skeleton->get_rest_pose_transforms()[root_index], transform{});
  EXPECT_EQ(skeleton->get_rest_pose_transforms()[child_0_index],
            (transform{float3{-1.0F, 0.0F, 0.0F}}));
  EXPECT_EQ(skeleton->get_rest_pose_transforms()[child_1_index],
            (transform{float3{1.0F, 0.0F, 0.0F}}));

  const clip* clip{project.get_resource<eely::clip>("test_clip")};
  EXPECT_EQ(clip->get_id(), "test_clip");
  EXPECT_EQ(clip->get_duration_s(), 8.0F);

  const std::vector<transform>& rest_transforms{skeleton->get_rest_pose_transforms()};

  std::unique_ptr<clip_player_base> player{clip->create_player()};
  EXPECT_EQ(player->get_duration_s(), 8.0F);

  skeleton_pose pose{*skeleton};

  // Remember acceptable errors
  // TODO: calculate and test compressed clips errors

  const auto calculate_joint_translation_acceptible_error = []() { return epsilon_default; };

  const auto calculate_joint_scale_acceptible_error = []() { return epsilon_default; };

  const float acceptible_error_quaternion = [/*compression_scheme*/]() {
    return epsilon_default;
  }();

  const float acceptible_error_root_translation{calculate_joint_translation_acceptible_error()};

  const float acceptible_error_root_scale{calculate_joint_scale_acceptible_error()};

  const float acceptible_error_child_0_translation{calculate_joint_translation_acceptible_error()};

  const float acceptible_error_child_0_scale{calculate_joint_scale_acceptible_error()};

  const float acceptible_error_child_1_translation{calculate_joint_translation_acceptible_error()};

  const float acceptible_error_child_1_scale{calculate_joint_scale_acceptible_error()};

  // Checks

  auto play_and_check_pose = [&](const float time_s) {
    player->play(time_s, pose);

    // root

    const transform& root_transform{pose.get_transform_joint_space(root_index)};
    expect_float3_near(root_transform.translation, root_translation_0,
                       acceptible_error_root_translation);
    expect_quaternion_near(root_transform.rotation, rest_transforms[root_index].rotation,
                           acceptible_error_quaternion);
    expect_float3_near(root_transform.scale, rest_transforms[root_index].scale,
                       acceptible_error_root_scale);

    // child_0

    const transform& child_0_transform{pose.get_transform_joint_space(child_0_index)};
    const float3 child_0_translation_expected = [&]() {
      if (time_s < 1.0F) {
        return float3_lerp(child_0_translation_0, child_0_translation_1, time_s);
      }

      if (time_s < 5.0F) {
        return float3_lerp(child_0_translation_1, child_0_translation_5, (time_s - 1.0F) / 4.0F);
      }

      if (time_s < 7.0F) {
        return float3_lerp(child_0_translation_5, child_0_translation_7, (time_s - 5.0F) / 2.0F);
      }

      if (time_s < 8.0F) {
        return float3_lerp(child_0_translation_7, child_0_translation_8, (time_s - 7.0F) / 1.0F);
      }

      return child_0_translation_8;
    }();
    const quaternion child_0_rotation_expected = [&]() {
      if (time_s < 3.0F) {
        return quaternion_slerp(child_0_rotation_0, child_0_rotation_3, time_s / 3.0F);
      }

      if (time_s < 4.0F) {
        return quaternion_slerp(child_0_rotation_3, child_0_rotation_4, (time_s - 3.0F) / 1.0F);
      }

      if (time_s < 7.0F) {
        return quaternion_slerp(child_0_rotation_4, child_0_rotation_7, (time_s - 4.0F) / 3.0F);
      }

      return child_0_rotation_7;
    }();
    const float3 child_0_scale_expect = [&]() {
      if (time_s < 1.0F) {
        return child_0_scale_1;
      }

      if (time_s < 2.0F) {
        return float3_lerp(child_0_scale_1, child_0_scale_2, (time_s - 1.0F) / 1.0F);
      }

      if (time_s < 4.0F) {
        return float3_lerp(child_0_scale_2, child_0_scale_4, (time_s - 2.0F) / 2.0F);
      }

      return child_0_scale_4;
    }();

    expect_float3_near(child_0_transform.translation, child_0_translation_expected,
                       acceptible_error_child_0_translation);
    expect_quaternion_near(child_0_transform.rotation, child_0_rotation_expected,
                           acceptible_error_quaternion);
    expect_float3_near(child_0_transform.scale, child_0_scale_expect,
                       acceptible_error_child_0_scale);

    // child_1

    const transform& child_1_transform{pose.get_transform_joint_space(child_1_index)};
    expect_float3_near(child_1_transform.translation, rest_transforms[child_1_index].translation,
                       acceptible_error_child_1_translation);
    expect_quaternion_near(child_1_transform.rotation, child_1_rotation_2,
                           acceptible_error_quaternion);
    expect_float3_near(child_1_transform.scale, rest_transforms[child_1_index].scale,
                       acceptible_error_child_1_scale);
  };

  play_and_check_pose(0.0F);
  play_and_check_pose(0.3F);
  play_and_check_pose(1.3F);
  play_and_check_pose(1.3F);
  play_and_check_pose(1.0F);
  play_and_check_pose(2.0F);
  play_and_check_pose(2.76F);
  play_and_check_pose(3.4F);
  play_and_check_pose(3.39F);
  play_and_check_pose(4.0F);
  play_and_check_pose(4.0F);
  play_and_check_pose(4.0F);
  play_and_check_pose(0.0F);
  play_and_check_pose(0.0F);
  play_and_check_pose(0.0F);
  play_and_check_pose(4.65F);
  play_and_check_pose(5.13F);
  play_and_check_pose(5.12F);
  play_and_check_pose(6.34F);
  play_and_check_pose(7.0F);
  play_and_check_pose(8.0F);
  play_and_check_pose(8.0F);

  static constexpr int random_samples = 200;
  std::mt19937 gen(seed);
  std::uniform_int_distribution<> distr(0, 100);
  for (int i{0}; i < random_samples; ++i) {
    const float percentage{gsl::narrow_cast<float>(distr(gen)) / 100.0F};
    const float time_s{player->get_duration_s() * percentage};
    play_and_check_pose(time_s);
  }
}

TEST(skeleton_and_clip, cook_and_play)
{
  test_skeleton_and_cip_with_scheme(eely::clip_compression_scheme::none);
}