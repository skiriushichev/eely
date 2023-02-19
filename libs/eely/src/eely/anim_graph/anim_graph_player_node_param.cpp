#include "eely/anim_graph/anim_graph_player_node_param.h"

#include "eely/anim_graph/anim_graph_node_param.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <any>

namespace eely::internal {
anim_graph_player_node_param::anim_graph_player_node_param(const int id, string_id param_id)
    : anim_graph_player_node_base{anim_graph_node_type::param, id}, _param_id{std::move(param_id)}
{
}

void anim_graph_player_node_param::compute_impl(const anim_graph_player_context& context,
                                                std::any& out_result)
{
  anim_graph_player_node_base::compute_impl(context, out_result);

  // TODO: a less verbose way to convert variant to any?
  // This is ugly.

  const param_value& value{context.params.get(_param_id)};

  switch (value.index()) {
    case 0: {
      static_assert(std::is_same_v<std::variant_alternative_t<0, param_value>, std::monostate>);
      out_result = {};
    } break;

    case 1: {
      static_assert(std::is_same_v<std::variant_alternative_t<1, param_value>, int>);
      out_result = std::get<int>(value);
    } break;

    case 2: {
      static_assert(std::is_same_v<std::variant_alternative_t<2, param_value>, float>);
      out_result = std::get<float>(value);
    } break;

    case 3: {
      static_assert(std::is_same_v<std::variant_alternative_t<3, param_value>, bool>);
      out_result = std::get<bool>(value);
    } break;

    default: {
      throw std::runtime_error("Unknown param value type");
    } break;
  }
}
}  // namespace eely::internal