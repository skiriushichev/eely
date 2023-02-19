#include "eely/anim_graph/anim_graph_player_node_state.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_state_condition.h"
#include "eely/anim_graph/anim_graph_player_node_state_transition.h"
#include "eely/base/assert.h"
#include "eely/base/base_utils.h"

#include <any>
#include <vector>

namespace eely::internal {
anim_graph_player_node_state::anim_graph_player_node_state(const int id, string_id name)
    : anim_graph_player_node_pose_base{anim_graph_node_type::state, id}, _name{std::move(name)}
{
  set_phase_rules(phase_rules::copy);
}

void anim_graph_player_node_state::update_duration(const anim_graph_player_context& context)
{
  anim_graph_player_node_pose_base::update_duration(context);

  _pose_node->update_duration(context);
  set_duration_s(_pose_node->get_duration_s());
}

void anim_graph_player_node_state::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  if (_pose_node != nullptr) {
    out_descendants.push_back(_pose_node);
    _pose_node->collect_descendants(out_descendants);
  }

  for (const anim_graph_player_node_state_transition* transition : _out_transition_nodes) {
    EXPECTS(transition != nullptr);

    out_descendants.push_back(transition);
    transition->collect_descendants(out_descendants);
  }
}

anim_graph_player_node_pose_base* anim_graph_player_node_state::get_pose_node() const
{
  return _pose_node;
}

void anim_graph_player_node_state::set_pose_node(anim_graph_player_node_pose_base* pose_node)
{
  _pose_node = pose_node;
  set_phase_copy_source(_pose_node);
}

const std::vector<anim_graph_player_node_state_transition*>&
anim_graph_player_node_state::get_out_transitions() const
{
  return _out_transition_nodes;
}

void anim_graph_player_node_state::set_out_transitions(
    std::vector<anim_graph_player_node_state_transition*> transitions)
{
  EXPECTS(std::all_of(transitions.begin(), transitions.end(),
                      [](const auto* t) { return t != nullptr; }));
  _out_transition_nodes = std::move(transitions);
}

void anim_graph_player_node_state::update_breakpoints()
{
  _breakpoints.clear();

  std::vector<const anim_graph_player_node_base*> conditions;

  for (const anim_graph_player_node_state_transition* transition : _out_transition_nodes) {
    const auto* condition_root{transition->get_condition_node()};
    EXPECTS(condition_root != nullptr);

    conditions.push_back(condition_root);
    condition_root->collect_descendants(conditions);

    for (const auto* condition : conditions) {
      if (condition->get_type() != anim_graph_node_type::state_condition) {
        continue;
      }

      const auto* state_condition{
          polymorphic_downcast<const anim_graph_player_node_state_condition*>(condition)};

      const std::optional<float> condition_phase{state_condition->get_phase()};

      if (!condition_phase.has_value()) {
        continue;
      }

      const float breakpoint{condition_phase.value()};

      auto breakpoint_find_iter{std::find(_breakpoints.begin(), _breakpoints.end(), breakpoint)};
      if (breakpoint_find_iter != _breakpoints.end()) {
        continue;
      }

      _breakpoints.push_back(breakpoint);
    }

    conditions.clear();
  }

  std::sort(_breakpoints.begin(), _breakpoints.end());
}

const std::vector<float>& anim_graph_player_node_state::get_breakpoints() const
{
  return _breakpoints;
}

void anim_graph_player_node_state::compute_impl(const anim_graph_player_context& context,
                                                std::any& out_result)
{
  EXPECTS(_pose_node != nullptr);

  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  out_result = _pose_node->compute(context);

  apply_next_phase(context);
}
}  // namespace eely::internal