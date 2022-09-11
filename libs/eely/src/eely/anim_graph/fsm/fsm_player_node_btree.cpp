#include "eely/anim_graph/fsm/fsm_player_node_btree.h"

#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
fsm_player_node_btree::fsm_player_node_btree() : fsm_player_node_base(fsm_player_node_type::btree)
{
}

fsm_player_node_btree::fsm_player_node_btree(const btree& btree)
    : fsm_player_node_base(fsm_player_node_type::btree), _btree_player{btree}
{
}

void fsm_player_node_btree::prepare(const anim_graph_context& context)
{
  fsm_player_node_base::prepare(context);

  _btree_player.prepare(context);
  set_duration_s(_btree_player.get_duration_s());
}

gsl::index fsm_player_node_btree::enqueue_job(const anim_graph_context& context)
{
  gsl::index result{_btree_player.enqueue_job(context)};
  set_phase(_btree_player.get_phase());
  return result;
}
}  // namespace eely::internal