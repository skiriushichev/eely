#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"

#include <memory>
#include <optional>

namespace eely {
// Node that calculates if a state matches specified conditions.
class anim_graph_node_state_condition : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_state_condition(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_state_condition(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get required state phase.
  [[nodiscard]] std::optional<float> get_phase() const;

  // Set required state phase.
  void set_phase(std::optional<float> value);

private:
  std::optional<float> _phase;
};
}  // namespace eely