#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <vector>

namespace eely::internal {
// Runtime player for a blendtree.
// Player serves as an entry point to the blendtree,
// owning all nodes and redirecting requests to the root node.
class btree_player final {
public:
  // Construct empty player.
  explicit btree_player() = default;

  // Construct a new player for specified blendtree.
  explicit btree_player(const btree& btree);

  // Prepare the tree's root node.
  void prepare(const anim_graph_context& context);

  // Enqueue root node's job.
  gsl::index enqueue_job(const anim_graph_context& context);

  // Return root node's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Return's root node's current phase.
  [[nodiscard]] float get_phase() const;

private:
  std::vector<internal::btree_player_node_uptr> _nodes;
  internal::btree_player_node_base* _root_node{nullptr};
};
}  // namespace eely::internal