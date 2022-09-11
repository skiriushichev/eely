#include "eely/anim_graph/btree/btree_player_node_fsm.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/anim_graph/fsm/fsm.h"
#include "eely/anim_graph/fsm/fsm_player.h"
#include "eely/job/job_queue.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
btree_player_node_fsm::btree_player_node_fsm(const fsm& fsm) : _player(fsm) {}

void btree_player_node_fsm::prepare(const anim_graph_context& context)
{
  _player.prepare(context);

  set_duration_s(_player.get_duration_s());
}

gsl::index btree_player_node_fsm::enqueue_job(const anim_graph_context& context)
{
  gsl::index result{_player.enqueue_job(context)};
  set_phase(_player.get_phase());
  return result;
}
}  // namespace eely::internal