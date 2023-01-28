#pragma once

#include "eely/anim_graph/anim_graph_node_param_comparison.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/string_id.h"
#include "eely/params/params.h"

#include <any>

namespace eely::internal {
// Runtime version of `anim_graph_node_param_comparison`.
class anim_graph_player_node_param_comparison final : public anim_graph_player_node_base {
public:
  // Construct node for specific parameter and value.
  explicit anim_graph_player_node_param_comparison(string_id param_id,
                                                   const param_value& value,
                                                   anim_graph_node_param_comparison::op op);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  string_id _param_id;
  param_value _value;
  anim_graph_node_param_comparison::op _op;
};
}  // namespace eely::internal