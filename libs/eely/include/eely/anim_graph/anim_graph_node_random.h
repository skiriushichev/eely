#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"

#include <memory>
#include <vector>

namespace eely {
// Pose node that selects a random child to play.
// Once selected node is played out fully, a new one is selected.
class anim_graph_node_random : public anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_random(int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_random(internal::bit_reader& reader);

  void serialize(internal::bit_writer& writer) const override;

  [[nodiscard]] anim_graph_node_uptr clone() const override;

  // Get list of children nodes.
  [[nodiscard]] const std::vector<int>& get_children_nodes() const;

  // Get modifiable list of children nodes.
  [[nodiscard]] std::vector<int>& get_children_nodes();

private:
  std::vector<int> _children_nodes;
};
}  // namespace eely