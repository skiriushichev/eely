#include "eely/skeleton/skeleton_pose.h"

#include "eely/math/transform.h"

#include <gsl/util>

#include <limits>
#include <optional>
#include <vector>

namespace eely {
skeleton_pose::skeleton_pose(const skeleton& skeleton) : _skeleton{&skeleton}
{
  reset();
}

void skeleton_pose::reset()
{
  const gsl::index joints_count{_skeleton->get_joints_count()};

  _transforms_joint_space.resize(joints_count);
  _transforms_joint_space = _skeleton->get_rest_pose_transforms();

  _transforms_object_space.resize(joints_count);

  if (joints_count > 0) {
    _shallow_changed_joint_index = 0;
  }
  else {
    _shallow_changed_joint_index = std::nullopt;
  }
}

void skeleton_pose::recalculate_object_space_transforms() const
{
  if (!_shallow_changed_joint_index.has_value()) {
    return;
  }

  for (gsl::index index{_shallow_changed_joint_index.value()};
       index < _transforms_joint_space.size(); ++index) {
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
}  // namespace eely