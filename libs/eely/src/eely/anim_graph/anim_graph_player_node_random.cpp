#include "eely/anim_graph/anim_graph_player_node_random.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/base/assert.h"

#include <gsl/narrow>

#include <any>
#include <random>
#include <vector>

namespace eely::internal {
anim_graph_player_node_random::anim_graph_player_node_random(const int id)
    : anim_graph_player_node_pose_base{anim_graph_node_type::random, id},
      _random_generator{std::random_device{}()}
{
  set_phase_rules(phase_rules::copy);
}

void anim_graph_player_node_random::update_duration(const anim_graph_player_context& context)
{
  anim_graph_player_node_pose_base::update_duration(context);

  if (_selected_node == nullptr) {
    select_node();
  }

  // We only switch to a new node during `compute` call, because
  //  - we do not know exact phase yet if we're in a sync mode
  //  - if we switch to a new node here, this node will never report a 1.0F phase,
  //    which can break transitions. This can probably be fixed by keeping previous phase
  //    along with a new one, if needed.

  _selected_node->update_duration(context);
  set_duration_s(_selected_node->get_duration_s());
}

void anim_graph_player_node_random::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  for (const anim_graph_player_node_pose_base* node : _children_nodes) {
    out_descendants.push_back(node);
    node->collect_descendants(out_descendants);
  }
}

const std::vector<anim_graph_player_node_pose_base*>&
anim_graph_player_node_random::get_children_nodes() const
{
  return _children_nodes;
}

void anim_graph_player_node_random::set_children_nodes(
    std::vector<anim_graph_player_node_pose_base*> children_nodes)
{
  EXPECTS(!children_nodes.empty());

  _children_nodes = std::move(children_nodes);

  _random_distribution =
      std::uniform_int_distribution<>(0, gsl::narrow<int>(_children_nodes.size() - 1));
}

void anim_graph_player_node_random::compute_impl(const anim_graph_player_context& context,
                                                 std::any& out_result)
{
  EXPECTS(!_children_nodes.empty());

  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  const float next_phase_unwrapped{_selected_node->get_next_phase_unwrapped(context)};
  if (next_phase_unwrapped > 1.0F) {
    select_node();
    _selected_node->update_duration(context);
  }

  out_result = _selected_node->compute(context);

  apply_next_phase(context);
}

void anim_graph_player_node_random::select_node()
{
  gsl::index index{_random_distribution(_random_generator)};
  EXPECTS(index >= 0 && index < std::ssize(_children_nodes));

  _selected_node = _children_nodes[index];
  set_phase_copy_source(_selected_node);
}
}  // namespace eely::internal