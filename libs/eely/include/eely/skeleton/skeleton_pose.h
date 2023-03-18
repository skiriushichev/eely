#pragma once

#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/skeleton/skeleton.h"

#include <gsl/narrow>
#include <gsl/pointers>
#include <gsl/util>

#include <optional>
#include <vector>

namespace eely {
// Represents pose of a skeleton.
// Pose is always linked to the specific skeleton, and should never outlive it.
class skeleton_pose final {
public:
  // Descripte type of a pose.
  // Type defines how this pose is used and what operations are valid.
  // E.g. you can only add additive poses to other poses.
  enum class type { absolute, additive };

  // Create pose for a skeleton.
  explicit skeleton_pose(const skeleton& skeleton, type pose_type = type::absolute);

  // Return transform of a joint with specified index, elative to its parent joint.
  [[nodiscard]] const transform& get_transform_joint_space(gsl::index index) const;

  // Set transform of a joint with specified index, relative to its parent joint.
  // If possible, use `sequence_*` methods instead.
  void set_transform_joint_space(gsl::index index, const transform& transform);

  // Set transform of a joint with specified index, relative to the object.
  void set_transform_object_space(gsl::index index, const transform& transform);

  // Start sequenced update of a pose.
  // After sequence is started, `set_transform_joint_space_sequenced` can be used.
  // `starting_index` reports shallowest index changed in a sequence.
  void sequence_start(gsl::index shallow_index);

  // Set transform of a joint with specified index, relative to its parent joint.
  // `sequence_start` must be called first.
  void sequence_set_transform_joint_space(gsl::index index, const transform& transform);

  // Set translation of a joint with specified index, relative to its parent joint.
  // `sequence_start` must be called first.
  void sequence_set_translation_joint_space(gsl::index index, const float3& translation);

  // Set rotation of a joint with specified index, relative to its parent joint.
  // `sequence_start` must be called first.
  void sequence_set_rotation_joint_space(gsl::index index, const quaternion& rotation);

  // Set scale of a joint with specified index, relative to its parent joint.
  // `sequence_start` must be called first.
  void sequence_set_scale_joint_space(gsl::index index, const float3& scale);

  // Return transform of a joint with specified index,
  // relative to the skeleton object.
  [[nodiscard]] const transform& get_transform_object_space(gsl::index index) const;

  // Return number of joints in a pose.
  [[nodiscard]] gsl::index get_joints_count() const;

  // Return skeleton this pose is for.
  [[nodiscard]] const skeleton& get_skeleton() const;

  // Reset pose.
  // Absolute poses reset to rest pose,
  // and additive poses reset to identities.
  void reset(type pose_type = type::absolute);

private:
  void recalculate_object_space_transforms() const;

  // Use `gsl::not_null` instead of a reference to make type move-assignable
  gsl::not_null<const skeleton*> _skeleton;

  std::vector<transform> _transforms_joint_space;

  mutable std::vector<transform> _transforms_object_space;

  // Index of a nearest (i.e. the most shallow) joint
  // that has changed after last call to `get_transform_object_space`.
  // Used to optimize object space recalculations (by ignoring joints that are unchanged).
  mutable std::optional<gsl::index> _shallow_changed_joint_index;
};

// Blend between two poses.
void skeleton_pose_blend(const skeleton_pose& p0,
                         const skeleton_pose& p1,
                         float weight,
                         skeleton_pose& out_result);

void skeleton_pose_add(const skeleton_pose& p0, const skeleton_pose& p1, skeleton_pose& out_result);

// Implementation

inline const transform& skeleton_pose::get_transform_joint_space(gsl::index index) const
{
  return _transforms_joint_space[index];
}

inline void skeleton_pose::set_transform_joint_space(const gsl::index index,
                                                     const transform& transform)
{
  _transforms_joint_space[index] = transform;

  _shallow_changed_joint_index = std::min(
      _shallow_changed_joint_index.value_or(std::numeric_limits<gsl::index>::max()), index);
}

inline void skeleton_pose::sequence_start(gsl::index shallow_index)
{
  _shallow_changed_joint_index = std::min(
      _shallow_changed_joint_index.value_or(std::numeric_limits<gsl::index>::max()), shallow_index);
}

inline void skeleton_pose::sequence_set_transform_joint_space(const gsl::index index,
                                                              const transform& transform)
{
  EXPECTS(float_near(quaternion_length(transform.rotation), 1.0F, 1e-3F));
  _transforms_joint_space[index] = transform;
}

inline void skeleton_pose::sequence_set_translation_joint_space(const gsl::index index,
                                                                const float3& translation)
{
  _transforms_joint_space[index].translation = translation;
}

inline void skeleton_pose::sequence_set_rotation_joint_space(const gsl::index index,
                                                             const quaternion& rotation)
{
  _transforms_joint_space[index].rotation = rotation;
}

inline void skeleton_pose::sequence_set_scale_joint_space(const gsl::index index,
                                                          const float3& scale)
{
  _transforms_joint_space[index].scale = scale;
}

inline const transform& skeleton_pose::get_transform_object_space(const gsl::index index) const
{
  recalculate_object_space_transforms();
  return _transforms_object_space[index];
}

inline gsl::index skeleton_pose::get_joints_count() const
{
  return std::ssize(_transforms_joint_space);
}

inline const skeleton& skeleton_pose::get_skeleton() const
{
  return *_skeleton;
}
}  // namespace eely