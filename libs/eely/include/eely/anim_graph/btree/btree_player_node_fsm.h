#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
class btree_player_node_fsm final : public btree_player_node_base {
public:
  explicit btree_player_node_fsm(const fsm& fsm);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  fsm_player _player;
};
}  // namespace eely::internal