#pragma once

#include "eely/anim_graph/anim_graph_node_param.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/string_id.h"

#include <any>

namespace eely::internal {
// Runtime version of `anim_graph_node_param`.
class anim_graph_player_node_param final : public anim_graph_player_node_base {
public:
  // Construct node for given parameter id.
  explicit anim_graph_player_node_param(int id, string_id param_id);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  string_id _param_id;
};
}  // namespace eely::internal