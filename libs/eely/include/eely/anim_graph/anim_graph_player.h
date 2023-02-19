#pragma once

#include "eely/anim_graph/anim_graph.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"
#include "eely/project/project.h"
#include "eely/skeleton/skeleton_pose.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace eely {
// Player for animation graphs.
class anim_graph_player final {
public:
  // Create a player for the specified graph.
  explicit anim_graph_player(const anim_graph& anim_graph);

  // Play a graph and put results into `out_pose`.
  void play(float dt_s, const params& params, skeleton_pose& out_pose);

  // Get list of all runtime nodes.
  [[nodiscard]] const std::vector<internal::anim_graph_player_node_uptr>& get_nodes() const;

  // Get runtime node by id.
  [[nodiscard]] const internal::anim_graph_player_node_base* get_player_node(int id) const;

  // Return `true` if specified node was active on last play.
  // Note: this is queried from player instead of a node itself,
  // because nodes don't receive any signals about being deactivated.
  [[nodiscard]] bool is_player_node_active(const internal::anim_graph_player_node_base& node) const;

private:
  internal::anim_graph_player_node_uptr create_player_node(const anim_graph_node_uptr& node);
  static void init_player_node(
      const anim_graph_node_uptr& node,
      internal::anim_graph_player_node_base* player_node,
      const std::unordered_map<int, internal::anim_graph_player_node_base*>& id_to_player_node);

  const project& _project;
  std::vector<internal::anim_graph_player_node_uptr> _nodes;
  internal::anim_graph_player_node_base* _root_node{nullptr};
  internal::job_queue _job_queue;
  int _play_counter{0};
};
}  // namespace eely