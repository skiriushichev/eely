#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource_uncooked.h"

#include <unordered_map>
#include <unordered_set>

namespace eely {
// Represents weight of each component of a joint.
struct joint_weight final {
  float translation{1.0F};
  float rotation{1.0F};
  float scale{1.0F};
};

// Skeleton mask is a collection of weights assigned to specific skeleton joints.
// Used to select how skeleton parts participate in operations,
// e.g. when layering animations on top of each other
// or when calculating additive clips for specific body parts (e.g. only for upper body).
class skeleton_mask_uncooked final : public resource_uncooked {
public:
  // Construct skeleton mask from a memory buffer.
  explicit skeleton_mask_uncooked(const project_uncooked& project, internal::bit_reader& reader);

  // Construct empty skeleton mask.
  // By default, all weights are equal to one (i.e. nothing is omitted).
  explicit skeleton_mask_uncooked(const project_uncooked& project, string_id id);

  void serialize(internal::bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Return skeleton id this mask is made for.
  [[nodiscard]] const string_id& get_target_skeleton_id() const;

  // Set skelton id this mask is made for.
  void set_target_skeleton_id(string_id skeleton_id);

  // Return mapping from joint id to its weight.
  // Weights should be in [0.0F, 1.0F] interval.
  [[nodiscard]] std::unordered_map<string_id, joint_weight>& get_weights();

  // Return mapping from joint id to its weight.
  [[nodiscard]] const std::unordered_map<string_id, joint_weight>& get_weights() const;

private:
  string_id _skeleton_id;
  std::unordered_map<string_id, joint_weight> _weights;
};
}  // namespace eely