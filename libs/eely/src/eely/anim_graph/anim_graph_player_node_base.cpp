#include "eely/anim_graph/anim_graph_player_node_base.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"

#include <optional>

namespace eely::internal {
anim_graph_player_node_base::anim_graph_player_node_base(const anim_graph_node_type type,
                                                         const int id)
    : _type{type}, _id{id}
{
}

std::any anim_graph_player_node_base::compute(const anim_graph_player_context& context)
{
  std::any result;
  compute_impl(context, result);

  return result;
}

void anim_graph_player_node_base::compute_impl(const anim_graph_player_context& context,
                                               std::any& /*out_result*/)
{
  register_play(context);
}
}  // namespace eely::internal