#include "eely/anim_graph/anim_graph_player_node_param_comparison.h"

#include "eely/anim_graph/anim_graph_node_param_comparison.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <any>

namespace eely::internal {
anim_graph_player_node_param_comparison::anim_graph_player_node_param_comparison(
    string_id param_id,
    const param_value& value,
    const anim_graph_node_param_comparison::op op)
    : anim_graph_player_node_base{anim_graph_node_type::param_comparison},
      _param_id{std::move(param_id)},
      _value{value},
      _op{op}
{
}

void anim_graph_player_node_param_comparison::compute_impl(const anim_graph_player_context& context,
                                                           std::any& out_result)
{
  anim_graph_player_node_base::compute_impl(context, out_result);

  switch (_op) {
    case anim_graph_node_param_comparison::op::equal: {
      out_result = context.params.get(_param_id) == _value;
    } break;

    case anim_graph_node_param_comparison::op::not_equal: {
      out_result = context.params.get(_param_id) != _value;
    } break;
  }
}
}  // namespace eely::internal