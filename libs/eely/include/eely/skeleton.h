#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/resource.h"
#include "eely/skeleton_uncooked.h"
#include "eely/string_id.h"
#include "eely/transform.h"

#include <gsl/util>

#include <limits>
#include <optional>
#include <vector>

namespace eely {
// Describes cooked skeleton.
class skeleton final : public resource {
public:
  // Construct a skeleton from a memory buffer.
  explicit skeleton(bit_reader& reader);

  // Construct a skeleton from an uncooked counterpart.
  explicit skeleton(const skeleton_uncooked& uncooked);

  // Serialize skeleton into memory buffer.
  void serialize(bit_writer& writer) const override;

  // Return number of joints in a skeleton.
  [[nodiscard]] gsl::index get_joints_count() const;

  // Return id of a joint with specified index.
  [[nodiscard]] const string_id& get_joint_id(gsl::index index) const;

  // Return id of a joint with specified index.
  [[nodiscard]] std::optional<gsl::index> get_joint_index(const string_id& id) const;

  // Return parent of a joint with specified index (if any).
  [[nodiscard]] std::optional<gsl::index> get_joint_parent_index(gsl::index index) const;

  // Return rest pose joint transforms.
  [[nodiscard]] const std::vector<transform>& get_rest_pose_transforms() const;

private:
  static constexpr gsl::index null_index{skeleton_uncooked::max_joints_count};

  std::vector<string_id> _joint_ids;
  std::vector<gsl::index> _joint_parents;
  std::vector<transform> _rest_pose;
};
}  // namespace eely