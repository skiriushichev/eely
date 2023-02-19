#pragma once

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/job/job_blend.h"

#include <any>
#include <optional>
#include <vector>

namespace eely::internal {
// Runtime version of `anim_graph_node_blend`.
class anim_graph_player_node_blend final : public anim_graph_player_node_pose_base {
public:
  struct pose_node_data final {
    anim_graph_player_node_pose_base& node;
    float factor{0.0F};
  };

  // Construct empty node.
  // Data must be filled via setters instead of ctor params,
  // because of the possible circular dependencies in a graph.
  explicit anim_graph_player_node_blend(int id);

  void update_duration(const anim_graph_player_context& context) override;

  void collect_descendants(
      std::vector<const anim_graph_player_node_base*>& out_descendants) const override;

  // Get list of pose nodes that participate in blending.
  [[nodiscard]] const std::vector<pose_node_data>& get_pose_nodes() const;

  // Get modifiable list of pose nodes that participate in blending.
  [[nodiscard]] std::vector<pose_node_data>& get_pose_nodes();

  // Get node that provides factor value for blending.
  [[nodiscard]] anim_graph_player_node_base* get_factor_node() const;

  // Set node that provides factor value for blending.
  void set_factor_node(anim_graph_player_node_base* node);

  // Return blend source node.
  const anim_graph_player_node_pose_base* get_current_source_node() const;

  // Return blend destination node.
  const anim_graph_player_node_pose_base* get_current_destination_node() const;

  // Return destination node's weight.
  // Source node's weight is 1.0F minus this value.
  float get_current_destination_node_weight() const;

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  // Update blended nodes based on a factor value.
  void select_blended_nodes(const anim_graph_player_context& context);

  std::vector<pose_node_data> _pose_nodes;
  anim_graph_player_node_base* _factor_node{nullptr};

  std::optional<float> _prev_factor;
  anim_graph_player_node_pose_base* _sorce_node{nullptr};
  anim_graph_player_node_pose_base* _destination_node{nullptr};
  float _destination_node_weight{0.0F};

  job_blend _job_blend;
};
}  // namespace eely::internal