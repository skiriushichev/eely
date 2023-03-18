#include "eely/skeleton/skeleton_pose.h"

#include "eely/base/assert.h"
#include "eely/math/transform.h"

#include <gsl/util>

#include <limits>
#include <optional>
#include <vector>

namespace eely {
skeleton_pose::skeleton_pose(const skeleton& skeleton, const type pose_type) : _skeleton{&skeleton}
{
  reset(pose_type);
}

void skeleton_pose::reset(const type pose_type)
{
  const gsl::index joints_count{_skeleton->get_joints_count()};

  if (pose_type == type::absolute) {
    _transforms_joint_space = _skeleton->get_rest_pose_transforms();
  }
  else {
    _transforms_joint_space.resize(joints_count);
    std::fill(_transforms_joint_space.begin(), _transforms_joint_space.end(), transform::identity);
  }

  if (joints_count > 0) {
    _shallow_changed_joint_index = 0;
  }
  else {
    _shallow_changed_joint_index = std::nullopt;
  }
}

void skeleton_pose::set_transform_object_space(const gsl::index index, const transform& transform)
{
  const std::optional<gsl::index> parent_index_opt{_skeleton->get_joint_parent_index(index)};
  if (!parent_index_opt.has_value()) {
    set_transform_joint_space(index, transform);
    return;
  }

  const eely::transform& parent_object_space_transform{
      get_transform_object_space(parent_index_opt.value())};
  set_transform_joint_space(index, transform_inverse(parent_object_space_transform) * transform);
}

void skeleton_pose::recalculate_object_space_transforms() const
{
  if (!_shallow_changed_joint_index.has_value()) {
    return;
  }

  const gsl::index joints_count{_skeleton->get_joints_count()};

  _transforms_object_space.resize(joints_count);

  for (gsl::index index{_shallow_changed_joint_index.value()}; index < joints_count; ++index) {
    const std::optional<gsl::index> parent_index{_skeleton->get_joint_parent_index(index)};
    if (parent_index.has_value()) {
      _transforms_object_space[index] =
          _transforms_object_space[parent_index.value()] * _transforms_joint_space[index];
    }
    else {
      _transforms_object_space[index] = _transforms_joint_space[index];
    }
  }

  _shallow_changed_joint_index = std::nullopt;
}

void skeleton_pose_blend(const skeleton_pose& p0,
                         const skeleton_pose& p1,
                         float weight,
                         skeleton_pose& out_result)
{
  EXPECTS(p0.get_joints_count() == p1.get_joints_count());
  EXPECTS(p0.get_joints_count() == out_result.get_joints_count());
  EXPECTS(&p0.get_skeleton() == &p1.get_skeleton());
  EXPECTS(&p0.get_skeleton() == &out_result.get_skeleton());

  const gsl::index joints_count{p0.get_joints_count()};

  out_result.sequence_start(0);

  for (gsl::index i{0}; i < joints_count; ++i) {
    const transform& t0{p0.get_transform_joint_space(i)};
    const transform& t1{p1.get_transform_joint_space(i)};

    out_result.sequence_set_translation_joint_space(
        i, float3_lerp(t0.translation, t1.translation, weight));
    out_result.sequence_set_rotation_joint_space(
        i, quaternion_slerp(t0.rotation, t1.rotation, weight));
    out_result.sequence_set_scale_joint_space(i, float3_lerp(t0.scale, t1.scale, weight));
  }
}

void skeleton_pose_add(const skeleton_pose& p0, const skeleton_pose& p1, skeleton_pose& out_result)
{
  EXPECTS(p0.get_joints_count() == p1.get_joints_count());
  EXPECTS(p0.get_joints_count() == out_result.get_joints_count());
  EXPECTS(&p0.get_skeleton() == &p1.get_skeleton());
  EXPECTS(&p0.get_skeleton() == &out_result.get_skeleton());

  const gsl::index joints_count{p0.get_joints_count()};

  out_result.sequence_start(0);

  for (gsl::index i{0}; i < joints_count; ++i) {
    const transform& t0{p0.get_transform_joint_space(i)};
    const transform& t1{p1.get_transform_joint_space(i)};
    out_result.sequence_set_transform_joint_space(i, t0 * t1);
  }
}
}  // namespace eely