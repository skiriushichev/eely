#pragma once

#include "eely/skeleton.h"
#include "eely/transform.h"

#include <gsl/pointers>
#include <gsl/util>

#include <optional>
#include <vector>

namespace eely {
// Represents pose of a skeleton.
// Pose is always linked to the specific skeleton,
// and should never outlive it.
class skeleton_pose final {
public:
  // Create pose for a skeleton.
  skeleton_pose(const skeleton& skeleton);

  // Return transform of a joint with specified index,
  // relative to its parent joint.
  [[nodiscard]] const transform& get_transform_joint_space(gsl::index index) const;

  // Set transform of a joint with specified index,
  // relative to its parent joint.
  void set_transform_joint_space(gsl::index index, const transform& transform);

  // Return transform of a joint with specified index,
  // relative to the skeleton object.
  [[nodiscard]] const transform& get_transform_object_space(gsl::index index) const;

  // Reset to the rest pose.
  void reset();

private:
  void recalculate_object_space_transforms() const;

  // Use `gsl::not_null` instead of a reference to make type move-assignable
  gsl::not_null<const skeleton*> _skeleton;

  std::vector<transform> _transforms_joint_space;

  mutable std::vector<transform> _transforms_object_space;

  // Index of a nearest (i.e. the most shallow) joint
  // that has changed after last call to `get_transform_object_space`
  mutable std::optional<gsl::index> _nearest_changed_joint_index;
};
}  // namespace eely