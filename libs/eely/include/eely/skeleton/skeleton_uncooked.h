#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/transform.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/util>

#include <optional>
#include <vector>

namespace eely {
// Describes an uncooked skeleton.
class skeleton_uncooked final : public resource_uncooked {
public:
  // Represents joint in a hierarchy.
  struct joint final {
    // Id of a joint.
    string_id id;

    // Parent index of a joint (if any).
    std::optional<gsl::index> parent_index;

    // Joint's transform in a rest pose (relative to its parent joint).
    transform rest_pose_transform;
  };

  // Construct an uncooked skeleton from a memory buffer.
  explicit skeleton_uncooked(bit_reader& reader);

  // Construct an empty uncooked skeleton.
  explicit skeleton_uncooked(const string_id& id);

  // Serialize skeleton into memory buffer.
  void serialize(bit_writer& writer) const override;

  // Get skeleton joints.
  [[nodiscard]] const std::vector<joint>& get_joints() const;

  // Set skeleton joints.
  void set_joints(std::vector<joint> joints);

private:
  std::vector<joint> _joints;
};
}  // namespace eely