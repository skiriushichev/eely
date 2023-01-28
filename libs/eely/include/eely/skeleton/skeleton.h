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
// Describes cooked skeleton.
class skeleton final : public resource {
public:
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

private:
  static constexpr gsl::index null_index{internal::joints_max_count};

  std::vector<string_id> _joint_ids;
  std::vector<gsl::index> _joint_parents;
  std::vector<transform> _rest_pose;
};
}  // namespace eely