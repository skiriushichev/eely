#include "eely/anim_graph/btree/btree_player.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree.h"
#include "eely/anim_graph/btree/btree_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <vector>

namespace eely::internal {
btree_player::btree_player(const btree& btree)
{
  using namespace eely::internal;

  const std::vector<anim_graph_node_uptr>& nodes{btree.get_nodes()};

  EXPECTS(!nodes.empty());

  // We need to resize nodes vector in advance,
  // because some nodes are initialized with pointers to other nodes,
  // and these pointers should not be invalidated.
  _nodes.resize(nodes.size());

  // For beldntree, we need to created nodes in reverse order,
  // so that when a parent is created its children were already there
  for (gsl::index i{std::ssize(nodes) - 1}; i >= 0; --i) {
    const anim_graph_node_uptr& node = nodes[i];
    EXPECTS(node != nullptr);

    _nodes[i] = btree_player_node_create(btree.get_project(), *node, _nodes);
  }

  // In a cooked blendtree, root is always at the zero.
  _root_node = _nodes[0].get();
}

void btree_player::prepare(const anim_graph_context& context)
{
  _root_node->prepare(context);
}

gsl::index btree_player::enqueue_job(const anim_graph_context& context)
{
  return _root_node->enqueue_job(context);
}

float btree_player::get_duration_s() const
{
  return _root_node->get_duration_s();
}

float btree_player::get_phase() const
{
  return _root_node->get_phase();
}
}  // namespace eely::internal