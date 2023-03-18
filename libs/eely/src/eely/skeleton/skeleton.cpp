#include "eely/skeleton/skeleton.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/elliptical_cone.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <limits>
#include <optional>
#include <vector>

namespace eely {
namespace internal {
template <>
skeleton::constraint bit_reader_read<skeleton::constraint>(bit_reader& reader)
{
  skeleton::constraint constraint;
  constraint.parent_constraint_delta = bit_reader_read<quaternion>(reader);
  constraint.child_constraint_delta = bit_reader_read<quaternion>(reader);
  constraint.limit_twist_rad = bit_reader_read<std::optional<float>>(reader);
  constraint.limit_swing_y_rad = bit_reader_read<std::optional<float>>(reader);
  constraint.limit_swing_z_rad = bit_reader_read<std::optional<float>>(reader);
  constraint.distance = bit_reader_read<float>(reader);

  return constraint;
}

void bit_writer_write(bit_writer& writer, const skeleton::constraint& value)
{
  bit_writer_write(writer, value.parent_constraint_delta);
  bit_writer_write(writer, value.child_constraint_delta);
  bit_writer_write(writer, value.limit_twist_rad);
  bit_writer_write(writer, value.limit_swing_y_rad);
  bit_writer_write(writer, value.limit_swing_z_rad);
  bit_writer_write(writer, value.distance);
}

template <>
inline skeleton::mapping bit_reader_read<skeleton::mapping>(bit_reader& reader)
{
  skeleton::mapping mapping;

  mapping.left_shoulder = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.left_arm = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.left_forearm = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.left_hand = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);

  mapping.right_shoulder = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.right_arm = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.right_forearm = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
  mapping.right_hand = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);

  return mapping;
}

void bit_writer_write(bit_writer& writer, const skeleton::mapping& value)
{
  bit_writer_write(writer, value.left_shoulder, bits_joints_count);
  bit_writer_write(writer, value.left_arm, bits_joints_count);
  bit_writer_write(writer, value.left_forearm, bits_joints_count);
  bit_writer_write(writer, value.left_hand, bits_joints_count);

  bit_writer_write(writer, value.right_shoulder, bits_joints_count);
  bit_writer_write(writer, value.right_arm, bits_joints_count);
  bit_writer_write(writer, value.right_forearm, bits_joints_count);
  bit_writer_write(writer, value.right_hand, bits_joints_count);
}
}  // namespace internal

skeleton::skeleton(const project& project, const skeleton_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  using namespace eely::internal;

  const std::vector<skeleton_uncooked::joint>& joints{uncooked.get_joints()};

  const gsl::index joints_count{std::ssize(joints)};

  EXPECTS(joints_count > 0);
  EXPECTS(joints_count < joints_max_count);

  _joint_ids.resize(joints_count);
  _joint_parents.resize(joints_count);
  _rest_pose.resize(joints_count);
  _constraints.resize(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_ids[i] = joints[i].id;

    const std::optional<gsl::index> parent_index{joints[i].parent_index};
    _joint_parents[i] = parent_index.value_or(null_index);

    _rest_pose[i] = joints[i].rest_pose_transform;
  }

  for (gsl::index i{0}; i < joints_count; ++i) {
    const std::optional<skeleton_uncooked::constraint> constraint_uncooked_opt{
        joints[i].constraint};

    skeleton::constraint constraint_cooked;

    // Precalculate bone length to avoid doing that on every IK iteration

    skeleton_pose rest_pose{*this};

    const transform& joint_space_transform{rest_pose.get_transform_joint_space(i)};

    std::optional<gsl::index> parent_index{get_joint_parent_index(i)};
    if (parent_index.has_value()) {
      constraint_cooked.distance =
          float3_distance(rest_pose.get_transform_joint_space(parent_index.value()).translation,
                          joint_space_transform.translation);
    }

    if (constraint_uncooked_opt.has_value()) {
      constraint_cooked.parent_constraint_delta = constraint_uncooked_opt->parent_constraint_delta;

      // Calculate child's twist axis as an average direction to its children

      float3 children_sum;
      for (gsl::index child_index{0}; child_index < joints_count; ++child_index) {
        if (get_joint_parent_index(child_index) != i) {
          continue;
        }

        children_sum += rest_pose.get_transform_object_space(child_index).translation -
                        rest_pose.get_transform_object_space(i).translation;
      }

      // Calculate constraint delta to this twist axis
      // with minimal difference from a parent constraint frame

      const transform& parent_object_space_transform{
          rest_pose.get_transform_object_space(parent_index.value())};

      [[maybe_unused]] float3 prev_x{
          vector_rotate(float3::x_axis, parent_object_space_transform.rotation)};

      const quaternion parent_constraint_frame{parent_object_space_transform.rotation *
                                               constraint_cooked.parent_constraint_delta};

      quaternion child_constraint_frame;

      if (!float3_near(children_sum, float3::zeroes)) {
        float3 twist_axis{vector_normalized(children_sum)};
        float3 parent_constraint_frame_x_axis{
            vector_rotate(float3::x_axis, parent_constraint_frame)};

        const quaternion parent_constraint_frame_delta{
            quaternion_shortest_arc(parent_constraint_frame_x_axis, twist_axis)};

        child_constraint_frame = parent_constraint_frame * parent_constraint_frame_delta;
      }
      else {
        child_constraint_frame = parent_constraint_frame;
      }

      // Calculate constraint frame in joint space
      quaternion child_constraint_frame_joint_space{
          quaternion_inverse(parent_object_space_transform.rotation) * child_constraint_frame};

      // Calculate delta to constraint frame
      constraint_cooked.child_constraint_delta =
          quaternion_inverse(joint_space_transform.rotation) * child_constraint_frame_joint_space;

      constraint_cooked.limit_twist_rad = constraint_uncooked_opt->limit_twist_rad;
      constraint_cooked.limit_swing_y_rad = constraint_uncooked_opt->limit_swing_y_rad;
      constraint_cooked.limit_swing_z_rad = constraint_uncooked_opt->limit_swing_z_rad;
    }
    _constraints[i] = constraint_cooked;
  }

  const skeleton_uncooked::mapping& uncooked_mapping{uncooked.get_mapping()};

  _mapping.left_shoulder = uncooked.get_joint_index(uncooked_mapping.left_shoulder);
  _mapping.left_arm = uncooked.get_joint_index(uncooked_mapping.left_arm);
  _mapping.left_forearm = uncooked.get_joint_index(uncooked_mapping.left_forearm);
  _mapping.left_hand = uncooked.get_joint_index(uncooked_mapping.left_hand);

  _mapping.right_shoulder = uncooked.get_joint_index(uncooked_mapping.right_shoulder);
  _mapping.right_arm = uncooked.get_joint_index(uncooked_mapping.right_arm);
  _mapping.right_forearm = uncooked.get_joint_index(uncooked_mapping.right_forearm);
  _mapping.right_hand = uncooked.get_joint_index(uncooked_mapping.right_hand);
}

skeleton::skeleton(const project& project, internal::bit_reader& reader) : resource(project, reader)
{
  using namespace eely::internal;

  const auto joints_count{bit_reader_read<gsl::index>(reader, bits_joints_count)};

  _joint_ids.resize(joints_count);
  _joint_parents.resize(joints_count);
  _rest_pose.resize(joints_count);
  _constraints.resize(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_ids[i] = bit_reader_read<string_id>(reader);
    _joint_parents[i] = bit_reader_read<gsl::index>(reader, bits_joints_count);
    _rest_pose[i] = bit_reader_read<transform>(reader);
    _constraints[i] = bit_reader_read<constraint>(reader);
  }

  _mapping = bit_reader_read<mapping>(reader);
}

void skeleton::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  const gsl::index joints_count{std::ssize(_joint_ids)};
  bit_writer_write(writer, joints_count, bits_joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    bit_writer_write(writer, _joint_ids[i]);
    bit_writer_write(writer, _joint_parents[i], bits_joints_count);
    bit_writer_write(writer, _rest_pose[i]);
    bit_writer_write(writer, _constraints[i]);
  }

  bit_writer_write(writer, _mapping);
}

gsl::index skeleton::get_joints_count() const
{
  return std::ssize(_joint_ids);
}

const string_id& skeleton::get_joint_id(gsl::index index) const
{
  return _joint_ids.at(index);
}

std::optional<gsl::index> skeleton::get_joint_index(const string_id& id) const
{
  const gsl::index joints_count{std::ssize(_joint_ids)};
  for (gsl::index i{0}; i < joints_count; ++i) {
    if (_joint_ids[i] == id) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<gsl::index> skeleton::get_joint_parent_index(gsl::index index) const
{
  gsl::index parent_index{_joint_parents.at(index)};
  return (parent_index == null_index) ? std::nullopt : std::optional<gsl::index>{parent_index};
}

const std::vector<transform>& skeleton::get_rest_pose_transforms() const
{
  return _rest_pose;
}

const skeleton::constraint& skeleton::get_constraint(const gsl::index index) const
{
  return _constraints.at(index);
}

const skeleton::mapping& skeleton::get_mapping() const
{
  return _mapping;
}

bool constraint_force(skeleton_pose& pose)
{
  bool constrained{false};

  const skeleton& skeleton{pose.get_skeleton()};
  for (gsl::index i{0}; i < skeleton.get_joints_count(); ++i) {
    constrained |= constraint_force(pose, i);
  }

  return constrained;
}

bool constraint_force(skeleton_pose& pose, gsl::index joint_index)
{
  const skeleton& skeleton{pose.get_skeleton()};
  const std::optional<gsl::index> parent_joint_index_opt{
      skeleton.get_joint_parent_index(joint_index)};
  if (!parent_joint_index_opt.has_value()) {
    // Root joints are not constrained.
    return false;
  }

  const skeleton::constraint& constraint{pose.get_skeleton().get_constraint(joint_index)};
  if (!constraint.limit_twist_rad.has_value() && !constraint.limit_swing_y_rad.has_value() &&
      !constraint.limit_swing_z_rad.has_value()) {
    // This joint is unconstrained.
    return false;
  }

  // Calculate delta between parent constraint frame and child constraint frame.
  // Constrain this delta to allowed limits.
  // Apply fixed delta and then recalculate transform from object space to joint space.

  const transform& parent_transform_object_space{
      pose.get_transform_object_space(parent_joint_index_opt.value())};
  const transform& child_transform_object_space{pose.get_transform_object_space(joint_index)};

  const quaternion parent_constraint_orientation{parent_transform_object_space.rotation *
                                                 constraint.parent_constraint_delta};

  quaternion child_constraint_orientation{child_transform_object_space.rotation *
                                          constraint.child_constraint_delta};

  quaternion delta{quaternion_normalized(quaternion_inverse(parent_constraint_orientation) *
                                         child_constraint_orientation)};

  const bool constrained{constraint_force(constraint, delta)};

  child_constraint_orientation = parent_constraint_orientation * delta;

  // TODO: inverses of predefined rotations can be precomputed during cooking
  const quaternion new_child_orientation{child_constraint_orientation *
                                         quaternion_inverse(constraint.child_constraint_delta)};

  const transform new_child_transform_joint_space{
      transform_inverse(parent_transform_object_space) *
      transform{child_transform_object_space.translation, new_child_orientation,
                child_transform_object_space.scale}};
  pose.set_transform_joint_space(joint_index, new_child_transform_joint_space);

  return constrained;
}

bool constraint_force(const skeleton::constraint& constraint, quaternion& q)
{
  using namespace eely::internal;

  bool constrained{false};

  quaternion twist{quaternion_extract(q, float3::x_axis)};
  quaternion swing_yz{q * quaternion_inverse(twist)};

  // Constraint twist

  if (constraint.limit_twist_rad.has_value()) {
    const float limit_twist_rad{constraint.limit_twist_rad.value()};

    float twist_angle_rad{quaternion_to_axis_angle(twist).second};

    if (twist_angle_rad < -limit_twist_rad) {
      constrained = true;
      twist_angle_rad = -limit_twist_rad;
    }

    if (twist_angle_rad > limit_twist_rad) {
      constrained = true;
      twist_angle_rad = limit_twist_rad;
    }

    twist = quaternion_from_axis_angle(1.0F, 0.0F, 0.0F, twist_angle_rad);
  }

  // Constraint swings

  if (constraint.limit_swing_y_rad.has_value() && constraint.limit_swing_z_rad.has_value()) {
    const float limit_swing_y_rad{constraint.limit_swing_y_rad.value()};
    const float limit_swing_z_rad{constraint.limit_swing_z_rad.value()};

    if (limit_swing_y_rad < pi && limit_swing_z_rad < pi) {
      // Constraint is form of an elliptical cone
      // Project onto it if needed

      const elliptical_cone cone{
          elliptical_cone_from_height_and_angles(1.0F, limit_swing_y_rad, limit_swing_z_rad)};

      float3 direction{vector_rotate(float3::x_axis, swing_yz)};

      if (!elliptical_cone_is_direction_inside(cone, direction)) {
        constrained = true;
        direction = elliptical_cone_project_direction(cone, direction);
        swing_yz = quaternion_shortest_arc(float3::x_axis, direction);
      }
    }
    else {
      // TODO: support for limits = pi and inversed elliptical cones (where angles are > pi)
      EXPECTS(false);
    }
  }
  else {
    auto swing_y{quaternion_extract(swing_yz, float3::y_axis)};
    auto swing_z{quaternion_extract(swing_yz, float3::y_axis)};

    if (constraint.limit_swing_y_rad.has_value()) {
      const float limit_swing_y_rad{constraint.limit_swing_y_rad.value()};

      float swing_y_angle_rad{quaternion_to_axis_angle(swing_y).second};

      if (swing_y_angle_rad < -limit_swing_y_rad) {
        constrained = true;
        swing_y_angle_rad = -limit_swing_y_rad;
      }

      if (swing_y_angle_rad > limit_swing_y_rad) {
        constrained = true;
        swing_y_angle_rad = limit_swing_y_rad;
      }

      swing_y = quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, swing_y_angle_rad);
    }
    else if (constraint.limit_swing_z_rad.has_value()) {
      const float limit_swing_z_rad{constraint.limit_swing_z_rad.value()};

      float swing_z_angle_rad{quaternion_to_axis_angle(swing_z).second};

      if (swing_z_angle_rad < -limit_swing_z_rad) {
        constrained = true;
        swing_z_angle_rad = -limit_swing_z_rad;
      }

      if (swing_z_angle_rad > limit_swing_z_rad) {
        constrained = true;
        swing_z_angle_rad = limit_swing_z_rad;
      }

      swing_z = quaternion_from_axis_angle(0.0F, 0.0F, 1.0F, swing_z_angle_rad);
    }

    swing_yz = swing_z * swing_y;
  }

  q = swing_yz * twist;

  return constrained;
}
}  // namespace eely