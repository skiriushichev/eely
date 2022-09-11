#pragma once

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"
#include "eely/skeleton/skeleton_pose.h"

#include <variant>

namespace eely {
// Player for an animation graph.
// Graph can start execution from either blendtree or a state machine.
// Blendtrees and state machines can be set up recursively.
// I.e. blendtree can play another blendtree or a state machine,
// and a state machine can play another nested state machine etc.
class anim_graph_player final {
public:
  // Construct player that starts execution from a blendtree.
  explicit anim_graph_player(const btree& btree, const params& params);

  // Construct player that starts execution from a state machine.
  explicit anim_graph_player(const fsm& fsm, const params& params);

  // Play animation graph and construct final pose.
  void play(float dt_s, skeleton_pose& out_pose);

private:
  std::variant<internal::btree_player, internal::fsm_player> _player_variant;
  internal::job_queue _job_queue;
  anim_graph_context _context;
};
}  // namespace eely