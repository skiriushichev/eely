#include "eely/anim_graph/btree/btree_player_node_btree.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_player.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
btree_player_node_btree::btree_player_node_btree(const btree& btree) : _player{btree} {}

void btree_player_node_btree::prepare(const anim_graph_context& context)
{
  _player.prepare(context);

  set_duration_s(_player.get_duration_s());
}

gsl::index btree_player_node_btree::enqueue_job(const anim_graph_context& context)
{
  gsl::index result{_player.enqueue_job(context)};
  set_phase(_player.get_phase());
  return result;
}

}  // namespace eely::internal