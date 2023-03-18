#include "eely/skeleton/presets.h"

#include "eely/math/quaternion.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_uncooked.h"

namespace eely {
void mixamo_init(skeleton_uncooked& skeleton_uncooked)
{
  skeleton_uncooked::mapping& mapping{skeleton_uncooked.get_mapping()};

  mapping.left_shoulder = "mixamorig:LeftShoulder";
  mapping.left_arm = "mixamorig:LeftArm";
  mapping.left_forearm = "mixamorig:LeftForeArm";
  mapping.left_hand = "mixamorig:LeftHand";

  mapping.right_shoulder = "mixamorig:RightShoulder";
  mapping.right_arm = "mixamorig:RightArm";
  mapping.right_forearm = "mixamorig:RightForeArm";
  mapping.right_hand = "mixamorig:RightHand";

  // TODO: setup rest of the joints

  skeleton_uncooked.get_joint(mapping.left_arm)->constraint = skeleton_uncooked::constraint{
      .parent_constraint_delta = quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, -deg_to_rad(125.0F))};
  skeleton_uncooked.get_joint(mapping.left_forearm)->constraint = skeleton_uncooked::constraint{
      .parent_constraint_delta = quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, -deg_to_rad(110.0F))};

  humanoid_constraints_limits_init(skeleton_uncooked);
}

void humanoid_constraints_limits_init(skeleton_uncooked& skeleton_uncooked)
{
  const skeleton_uncooked::mapping& mapping{skeleton_uncooked.get_mapping()};

  // TODO: setup rest of the joints

  skeleton_uncooked::constraint& left_arm_constraint{
      skeleton_uncooked.get_joint(mapping.left_arm)->constraint.value()};
  left_arm_constraint.limit_swing_y_rad = deg_to_rad(130.0F);
  left_arm_constraint.limit_swing_z_rad = pi * 0.5F;
  left_arm_constraint.limit_twist_rad = pi / 6.0F;

  skeleton_uncooked::constraint& left_forearm_constraint{
      skeleton_uncooked.get_joint(mapping.left_forearm)->constraint.value()};
  left_forearm_constraint.limit_swing_y_rad = deg_to_rad(150.0F);
  left_forearm_constraint.limit_swing_z_rad = deg_to_rad(5.0F);
  left_forearm_constraint.limit_twist_rad = 0.0F;
}
}  // namespace eely