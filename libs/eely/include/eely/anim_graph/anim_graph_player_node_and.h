#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"

#include <any>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_and`.
class anim_graph_player_node_and final : public anim_graph_player_node_base {
public:
  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  anim_graph_player_node_and(int id);

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Return list of children nodes.
  [[nodiscard]] const std::vector<anim_graph_player_node_base*>& get_children_nodes() const;

  // Return modifiable list of children nodes.
  [[nodiscard]] std::vector<anim_graph_player_node_base*>& get_children_nodes();

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  std::vector<anim_graph_player_node_base*> _children_nodes;
};
}  // namespace eely::internal