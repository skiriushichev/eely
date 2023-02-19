#pragma once

#include "eely/anim_graph/anim_graph_node_speed.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"

#include <any>

namespace eely::internal {
// Runtime version of `anim_graph_node_speed`.
class anim_graph_player_node_speed final : public anim_graph_player_node_base {
public:
  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_speed(int id);

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Set node that will have its speed modified.
  void set_child_node(anim_graph_player_node_base* node);

  // Set node that provides speed factor.
  void set_speed_provider_node(anim_graph_player_node_base* node);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  anim_graph_player_node_base* _child_node{nullptr};
  anim_graph_player_node_base* _speed_provider_node{nullptr};
};
}  // namespace eely::internal