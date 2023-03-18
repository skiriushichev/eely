#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/transform.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/util>

#include <optional>
#include <vector>

namespace eely {
// Describes an uncooked skeleton.
class skeleton_uncooked final : public resource_uncooked {
public:
  // Mapping from skeleton joints to commonly used body parts.
  struct mapping final {
    string_id left_shoulder;
    string_id left_arm;
    string_id left_forearm;
    string_id left_hand;

    string_id right_shoulder;
    string_id right_arm;
    string_id right_forearm;
    string_id right_hand;
  };

  // Represents joint limits.
  struct constraint final {
    // Frame relative to which constraint is expressed.
    quaternion parent_constraint_delta;

    std::optional<float> limit_twist_rad;
    std::optional<float> limit_swing_y_rad;
    std::optional<float> limit_swing_z_rad;
  };

  // Represents joint in a hierarchy.
  struct joint final {
    // Id of a joint.
    string_id id;

    // Parent index of a joint (if any).
    std::optional<gsl::index> parent_index;

    // Joint's transform in a rest pose (relative to its parent joint).
    transform rest_pose_transform;

    // Joint's constraint.
    std::optional<constraint> constraint;
  };

  // Construct an uncooked skeleton from a memory buffer.
  explicit skeleton_uncooked(const project_uncooked& project, internal::bit_reader& reader);

  // Construct an empty uncooked skeleton.
  explicit skeleton_uncooked(const project_uncooked& project, string_id id);

  // Serialize skeleton into memory buffer.
  void serialize(internal::bit_writer& writer) const override;

  // Get read-only skeleton joints.
  [[nodiscard]] const std::vector<joint>& get_joints() const;

  // Get skeleton joints.
  [[nodiscard]] std::vector<joint>& get_joints();

  // Get read-only joint with specified id.
  [[nodiscard]] const joint* get_joint(const string_id& joint_id) const;

  // Get joint with specified id.
  [[nodiscard]] joint* get_joint(const string_id& joint_id);

  // Get index of a joint with specified id.
  [[nodiscard]] std::optional<gsl::index> get_joint_index(const string_id& joint_id) const;

  // Get skeleton's read-only mapping.
  [[nodiscard]] const mapping& get_mapping() const;

  // Get skeleton's mapping.
  [[nodiscard]] mapping& get_mapping();

private:
  std::vector<joint> _joints;
  mapping _mapping;
};
}  // namespace eely