#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/job/job_add.h"

#include <any>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_sum`.
class anim_graph_player_node_sum final : public anim_graph_player_node_pose_base {
public:
  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_sum(int id);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Get first node in a sum.
  [[nodiscard]] anim_graph_player_node_pose_base* get_first_node() const;

  // Set first node in a sum.
  void set_first_node(anim_graph_player_node_pose_base* node);

  // Get second node in a sum.
  [[nodiscard]] anim_graph_player_node_pose_base* get_second_node() const;

  // Set second node in a sum.
  void set_second_node(anim_graph_player_node_pose_base* node);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  anim_graph_player_node_pose_base* _first_node{nullptr};
  anim_graph_player_node_pose_base* _second_node{nullptr};

  job_add _job_add;
};
}  // namespace eely::internal