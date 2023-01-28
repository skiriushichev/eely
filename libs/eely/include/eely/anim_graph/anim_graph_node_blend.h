#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>
#include <vector>

namespace eely {
// Node that has N children pose nodes to blend together,
// and one children node that provides factor value for blending.
// Each child pose node is assigned a factor value.
// If provided factor value matches it exactly, only one node will be used,
// otherwise two nodes will be blended together with according weight.
class anim_graph_node_blend final : public anim_graph_node_base {
public:
  // Data for a child pose node that will be blended:
  // its id + assigned factor value.
  struct pose_node_data final {
    std::optional<int> id;
    float factor{0.0F};
  };

  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_blend(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_blend(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get list of pose nodes that participate in blending.
  [[nodiscard]] const std::vector<pose_node_data>& get_pose_nodes() const;

  // Get modifiable list of pose nodes that participate in blending.
  [[nodiscard]] std::vector<pose_node_data>& get_pose_nodes();

  // Get index of a node that provides factor value for blending.
  [[nodiscard]] std::optional<int> get_factor_node_id() const;

  // Set index of a node that provides factor value for blending.
  void set_factor_node_id(std::optional<int> id);

private:
  std::vector<pose_node_data> _pose_nodes;
  std::optional<int> _factor_node;
};
}  // namespace eely