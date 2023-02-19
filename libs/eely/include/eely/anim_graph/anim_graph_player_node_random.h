#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"

#include <any>
#include <random>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_random`.
class anim_graph_player_node_random final : public anim_graph_player_node_pose_base {
public:
  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  anim_graph_player_node_random(int id);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Return list of children nodes.
  [[nodiscard]] const std::vector<anim_graph_player_node_pose_base*>& get_children_nodes() const;

  // Set list of children nodes.
  void set_children_nodes(std::vector<anim_graph_player_node_pose_base*> children_nodes);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  void select_node();

  std::mt19937 _random_generator;
  std::uniform_int_distribution<> _random_distribution;

  std::vector<anim_graph_player_node_pose_base*> _children_nodes;
  anim_graph_player_node_pose_base* _selected_node{nullptr};
};
}  // namespace eely::internal