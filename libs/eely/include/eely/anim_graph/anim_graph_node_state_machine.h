#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"

#include <memory>
#include <vector>

namespace eely {
// Node that runs a state machine,
// where each state and transitions between them
// are represented as separate children nodes.
class anim_graph_node_state_machine final : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_state_machine(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_state_machine(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get a list of state nodes.
  [[nodiscard]] const std::vector<int>& get_state_nodes() const;

  // Get a modifieable list of state nodes.
  [[nodiscard]] std::vector<int>& get_state_nodes();

private:
  std::vector<int> _state_nodes;
};
}  // namespace eely