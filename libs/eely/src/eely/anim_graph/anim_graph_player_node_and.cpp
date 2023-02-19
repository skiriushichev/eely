#include "eely/anim_graph/anim_graph_player_node_and.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/assert.h"

#include <any>
#include <vector>

namespace eely::internal {
anim_graph_player_node_and::anim_graph_player_node_and(const int id)
    : anim_graph_player_node_base{anim_graph_node_type::and_logic, id}
{
}

void anim_graph_player_node_and::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  for (const anim_graph_player_node_base* node : _children_nodes) {
    out_descendants.push_back(node);
    node->collect_descendants(out_descendants);
  }
}

const std::vector<anim_graph_player_node_base*>& anim_graph_player_node_and::get_children_nodes()
    const
{
  return _children_nodes;
}

std::vector<anim_graph_player_node_base*>& anim_graph_player_node_and::get_children_nodes()
{
  return _children_nodes;
}

void anim_graph_player_node_and::compute_impl(const anim_graph_player_context& context,
                                              std::any& out_result)
{
  anim_graph_player_node_base::compute_impl(context, out_result);

  EXPECTS(!_children_nodes.empty());

  for (anim_graph_player_node_base* node : _children_nodes) {
    EXPECTS(node != nullptr);

    const bool result{std::any_cast<bool>(node->compute(context))};
    if (!result) {
      out_result = false;
      return;
    }
  }

  out_result = true;
}
}  // namespace eely::internal