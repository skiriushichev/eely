#include "eely/anim_graph/anim_graph_player_node_base.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"

#include <optional>

namespace eely::internal {
anim_graph_player_node_base::anim_graph_player_node_base(const anim_graph_node_type type)
    : _type{type}
{
}

std::any anim_graph_player_node_base::compute(const anim_graph_player_context& context)
{
  std::any result;
  compute_impl(context, result);

  return result;
}
}  // namespace eely::internal