#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <optional>
#include <vector>

namespace eely {
// Node that represents a state in a state machine.
class anim_graph_node_state final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_state(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_state(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get id if a node that calculates pose
  // when this state is active or is being transitioned from.
  [[nodiscard]] std::optional<int> get_pose_node() const;

  // Set id if a node that calculates pose
  // when this state is active or is being transitioned from.
  void set_pose_node(std::optional<int> value);

  // Get list of transitions from this state.
  [[nodiscard]] const std::vector<int>& get_out_transition_nodes() const;

  // Get modifiable list of transitions from this state.
  [[nodiscard]] std::vector<int>& get_out_transition_nodes();

private:
  std::optional<int> _pose_node;
  std::vector<int> _out_transition_nodes;
};
}  // namespace eely