#include "eely/anim_graph/anim_graph_player_node_speed.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/base/base_utils.h"

namespace eely::internal {
anim_graph_player_node_speed::anim_graph_player_node_speed(const int id)
    : anim_graph_player_node_base{anim_graph_node_type::speed, id}
{
}

void anim_graph_player_node_speed::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  if (_child_node != nullptr) {
    out_descendants.push_back(_child_node);
    _child_node->collect_descendants(out_descendants);
  }

  if (_speed_provider_node != nullptr) {
    out_descendants.push_back(_speed_provider_node);
    _speed_provider_node->collect_descendants(out_descendants);
  }
}

void anim_graph_player_node_speed::set_child_node(anim_graph_player_node_base* const node)
{
  _child_node = node;
}

void anim_graph_player_node_speed::set_speed_provider_node(anim_graph_player_node_base* const node)
{
  _speed_provider_node = node;
}

void anim_graph_player_node_speed::compute_impl(const anim_graph_player_context& context,
                                                std::any& out_result)
{
  EXPECTS(_child_node != nullptr);
  EXPECTS(_speed_provider_node != nullptr);

  anim_graph_player_node_base::compute_impl(context, out_result);

  anim_graph_player_context context_pass_on{context};
  context_pass_on.dt_s *= std::any_cast<float>(_speed_provider_node->compute(context));

  out_result = _child_node->compute(context_pass_on);
}
}  // namespace eely::internal