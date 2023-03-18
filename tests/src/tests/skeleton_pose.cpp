#include "tests/test_utils.h"

#include "eely/math/quaternion.h"
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_pose.h>

#include <gtest/gtest.h>

TEST(skeleton_pose, skeleton_pose)
{
  using namespace eely;
  using namespace eely::internal;

  std::array<std::byte, 1024> buffer;

  constexpr gsl::index root_index{0};
  constexpr gsl::index child_0_index{1};
  constexpr gsl::index child_1_index{2};

  // Cook test data
  {
    project_uncooked project_uncooked(measurement_unit::meters,
                                      axis_system::y_up_x_right_z_forward);

    auto& skeleton_uncooked =
        project_uncooked.add_resource<eely::skeleton_uncooked>("test_skeleton");
    skeleton_uncooked.get_joints() = {
        {.id = "root", .parent_index = std::nullopt, .rest_pose_transform = transform{}},
        {.id = "child_0",
         .parent_index = 0,
         .rest_pose_transform = transform{float3{-1.0F, 0.0F, 0.0F}}},
        {.id = "child_1",
         .parent_index = 0,
         .rest_pose_transform = transform{float3{1.0F, 0.0F, 0.0F}}}};

    EXPECT_EQ(skeleton_uncooked.get_id(), "test_skeleton");

    project::cook(project_uncooked, buffer);
  }

  project project{buffer};

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

  skeleton_pose pose{*skeleton};

  EXPECT_EQ(pose.get_transform_joint_space(root_index),
            skeleton->get_rest_pose_transforms()[root_index]);
  EXPECT_EQ(pose.get_transform_joint_space(child_0_index),
            skeleton->get_rest_pose_transforms()[child_0_index]);
  EXPECT_EQ(pose.get_transform_joint_space(child_1_index),
            skeleton->get_rest_pose_transforms()[child_1_index]);

  // Since root transform is identity,
  // default object space transforms for both children should be the same as in joint space
  EXPECT_EQ(pose.get_transform_object_space(root_index),
            skeleton->get_rest_pose_transforms()[root_index]);
  EXPECT_EQ(pose.get_transform_object_space(child_0_index),
            skeleton->get_rest_pose_transforms()[child_0_index]);
  EXPECT_EQ(pose.get_transform_object_space(child_1_index),
            skeleton->get_rest_pose_transforms()[child_1_index]);

  transform root_transform{float3{0.0F, 1.0F, 0.0F},
                           quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, -pi / 2.0F)};
  pose.set_transform_joint_space(root_index, root_transform);
  EXPECT_EQ(pose.get_transform_joint_space(root_index), root_transform);
  EXPECT_EQ(pose.get_transform_joint_space(child_0_index),
            skeleton->get_rest_pose_transforms()[child_0_index]);
  EXPECT_EQ(pose.get_transform_joint_space(child_1_index),
            skeleton->get_rest_pose_transforms()[child_1_index]);
  EXPECT_EQ(pose.get_transform_object_space(root_index), root_transform);
  expect_transform_near(pose.get_transform_object_space(child_0_index),
                        transform{float3{0.0F, 1.0F, -1.0F}, root_transform.rotation});
  expect_transform_near(pose.get_transform_object_space(child_1_index),
                        transform{float3{0.0F, 1.0F, 1.0F}, root_transform.rotation});

  root_transform =
      transform{float3{0.0F, 0.0F, 1.0F}, quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, pi / 2.0F)};
  pose.set_transform_joint_space(root_index, root_transform);
  EXPECT_EQ(pose.get_transform_joint_space(root_index), root_transform);
  EXPECT_EQ(pose.get_transform_joint_space(child_0_index),
            skeleton->get_rest_pose_transforms()[child_0_index]);
  EXPECT_EQ(pose.get_transform_joint_space(child_1_index),
            skeleton->get_rest_pose_transforms()[child_1_index]);
  EXPECT_EQ(pose.get_transform_object_space(root_index), root_transform);
  expect_transform_near(pose.get_transform_object_space(child_0_index),
                        transform{float3{0.0F, 0.0F, 2.0F}, root_transform.rotation});
  expect_transform_near(pose.get_transform_object_space(child_1_index),
                        transform{float3{0.0F, 0.0F, 0.0F}, root_transform.rotation});
}