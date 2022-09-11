#pragma once

#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
// State machine node that plays a blendtree.
class fsm_player_node_btree final : public fsm_player_node_base {
public:
  // Construct empty node.
  fsm_player_node_btree();

  // Construct node that plays specified blendtree.
  fsm_player_node_btree(const btree& btree);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  btree_player _btree_player;
};
}  // namespace eely::internal