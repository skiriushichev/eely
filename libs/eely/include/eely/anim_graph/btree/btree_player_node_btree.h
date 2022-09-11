#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
class btree_player_node_btree final : public btree_player_node_base {
public:
  explicit btree_player_node_btree(const btree& btree);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  btree_player _player;
};
}  // namespace eely::internal