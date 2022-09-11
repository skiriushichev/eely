#pragma once

#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
// State machine node that plays another state machine underneath.
class fsm_player_node_fsm final : public fsm_player_node_base {
public:
  // Construct empty node.
  fsm_player_node_fsm();

  // Construct node that plays specified state machine.
  fsm_player_node_fsm(const fsm& fsm);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  void on_start(const anim_graph_context& context) override;

  fsm_player _fsm_player;
};
}  // namespace eely::internal