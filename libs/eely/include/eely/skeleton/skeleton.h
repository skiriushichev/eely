#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/transform.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/util>

#include <limits>
#include <optional>
#include <vector>

namespace eely {
// Forward declaration to cut circular dependency.
class skeleton_pose;

// Describes cooked skeleton.
class skeleton final : public resource {
public:
  // Mapping from skeleton joints to commonly used body parts.
  struct mapping final {
    std::optional<gsl::index> left_shoulder;
    std::optional<gsl::index> left_arm;
    std::optional<gsl::index> left_forearm;
    std::optional<gsl::index> left_hand;

    std::optional<gsl::index> right_shoulder;
    std::optional<gsl::index> right_arm;
    std::optional<gsl::index> right_forearm;
    std::optional<gsl::index> right_hand;
  };

  // Represents joint limits.
  struct constraint final {
    // Frame relative to which constraint is expressed.
    quaternion parent_constraint_delta;

    // Frame that is being constrained.
    // Relative to constrained joint's frame.
    quaternion child_constraint_delta;

    std::optional<float> limit_twist_rad;
    std::optional<float> limit_swing_y_rad;
    std::optional<float> limit_swing_z_rad;

    // Distance from parent to child,
    // i.e. bone length.
    float distance{0.0F};
  };

  // Construct a skeleton from a memory buffer.
  explicit skeleton(const project& project, internal::bit_reader& reader);

  // Construct a skeleton from an uncooked counterpart.
  explicit skeleton(const project& project, const skeleton_uncooked& uncooked);

  // Serialize skeleton into memory buffer.
  void serialize(internal::bit_writer& writer) const override;

  // Return number of joints in a skeleton.
  [[nodiscard]] gsl::index get_joints_count() const;

  // Return id of a joint with specified index.
  [[nodiscard]] const string_id& get_joint_id(gsl::index index) const;

  // Return index of a joint with specified id.
  [[nodiscard]] std::optional<gsl::index> get_joint_index(const string_id& id) const;

  // Return parent of a joint with specified index (if any).
  [[nodiscard]] std::optional<gsl::index> get_joint_parent_index(gsl::index index) const;

  // Return rest pose joint transforms.
  // These transforms are all relative to joint's parent
  // (or to object, if joint is a root).
  [[nodiscard]] const std::vector<transform>& get_rest_pose_transforms() const;

  // Get constraint of a joint with specified index.
  [[nodiscard]] const constraint& get_constraint(gsl::index index) const;

  // Get skeleton mapping.
  [[nodiscard]] const mapping& get_mapping() const;

private:
  static constexpr gsl::index null_index{internal::joints_max_count};

  std::vector<string_id> _joint_ids;
  std::vector<gsl::index> _joint_parents;
  std::vector<transform> _rest_pose;
  std::vector<constraint> _constraints;
  mapping _mapping;
};

// Force constraints on all joints in a pose.
// Return `true` if any joint was constrained from its previous transform.
bool constraint_force(skeleton_pose& pose);

// Force constraint on a given joint in a pose.
// Return `true` if a joint was constrained from its previous transform.
bool constraint_force(skeleton_pose& pose, gsl::index joint_index);

// Force constraint on a given orientation.
// Return `true` if constraint was indeed forced.
bool constraint_force(const skeleton::constraint& constraint, quaternion& q);
}  // namespace eely