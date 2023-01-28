#include "eely/anim_graph/anim_graph_player_node_state_machine.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_state.h"
#include "eely/anim_graph/anim_graph_player_node_state_transition.h"
#include "eely/base/base_utils.h"

#include <algorithm>
#include <any>
#include <vector>

namespace eely::internal {
// Disable warning on purpose.
// This seems to be the easiest way to give access to the current state machine.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const anim_graph_player_node_state_machine* anim_graph_player_node_state_machine::current{nullptr};

const anim_graph_player_node_state_machine* anim_graph_player_node_state_machine::get_current()
{
  EXPECTS(current != nullptr);
  return current;
}

anim_graph_player_node_state_machine::anim_graph_player_node_state_machine()
    : anim_graph_player_node_pose_base{anim_graph_node_type::state_machine}
{
  // State machine's phase is always equal to current state's phase.
  set_phase_rules(phase_rules::copy);
}

void anim_graph_player_node_state_machine::update_duration(const anim_graph_player_context& context)
{
  anim_graph_player_node_pose_base::update_duration(context);

  anim_graph_player_node_state_machine::current = this;

  // Ideally, we would like to report duration of a state machine
  // after all transitions have been checked and we calculated which state the machine is in.
  // However, when a state machine is running in a synchronized mode,
  // some transitions cannot be checked here fully (the ones that start at a specific phase).
  // This is because the phase will only be known when `compute` is called.
  //
  // We will still check all conditions here,
  // but in sync mode we will use previous phase instead of a new one.
  // We will reevaluate in `compute` to make sure that all up-to-date transitions do happen on this
  // frame, while reporting duration as precise as possible.
  //
  // Unfortunately, this means that synchronized state machines take more time to update
  // (since we check all conditions twice).

  const bool predict_phase{_current_node->get_type() == anim_graph_node_type::state_transition ||
                           !context.sync_enabled};

  if (predict_phase) {
    _current_node->update_duration(context);

    const float next_phase{_current_node->get_next_phase_unwrapped(context)};

    if (update_state(context, next_phase)) {
      // If we switched to another state, we must update duration of it as well
      _current_node->update_duration(context);
    }
  }
  else {
    update_state(context, get_phase());
    _current_node->update_duration(context);
  }

  // Update state machine's duration

  if (_current_node->get_type() == anim_graph_node_type::state_transition) {
    // When we're in a transition, state machine's duration must be the same as a destination node.
    // Transitions are not revealed to the outside world.

    auto* current_transition{
        polymorphic_downcast<anim_graph_player_node_state_transition*>(_current_node)};

    const anim_graph_player_node_state* current_destination{
        current_transition->get_current_destination_state_node()};

    set_duration_s(current_destination->get_duration_s());
  }
  else {
    set_duration_s(_current_node->get_duration_s());
  }

  update_phase_copy_source();

  anim_graph_player_node_state_machine::current = nullptr;
}

void anim_graph_player_node_state_machine::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  for (const anim_graph_player_node_state* node : _state_nodes) {
    EXPECTS(node != nullptr);

    out_descendants.push_back(node);
    node->collect_descendants(out_descendants);
  }
}

void anim_graph_player_node_state_machine::set_state_nodes(
    std::vector<anim_graph_player_node_state*> state_nodes)
{
  EXPECTS(!state_nodes.empty());
  EXPECTS(std::all_of(state_nodes.begin(), state_nodes.end(),
                      [](const auto* n) { return n != nullptr; }));

  _state_nodes = std::move(state_nodes);
  _current_node = _state_nodes[0];
}

const anim_graph_player_node_state*
anim_graph_player_node_state_machine::get_transition_source_candidate() const
{
  return _transition_source_candidate;
}

float anim_graph_player_node_state_machine::get_transition_source_candidate_phase() const
{
  return _transition_source_candidate_phase;
}

anim_graph_player_node_state* anim_graph_player_node_state_machine::get_transition_source() const
{
  return _transition_source;
}

void anim_graph_player_node_state_machine::compute_impl(const anim_graph_player_context& context,
                                                        std::any& out_result)
{
  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  anim_graph_player_node_state_machine::current = this;

  if (context.sync_enabled) {
    // See comment in `update_duration` on why state is updated here as well for synced mode.
    // We can actually do that only if we're in a state that has breakpoints
    // (i.e. this state has transitions happening at specific phases).
    // Otherwise phase does not affect transitions from this state
    // And we already did check for these in `update_duration`.
    if (_current_node->get_type() == anim_graph_node_type::state) {
      const auto* current_state =
          polymorphic_downcast<anim_graph_player_node_state*>(_current_node);

      if (!current_state->get_breakpoints().empty()) {
        update_state(context, get_phase());
      }
    }
  }

  out_result = _current_node->compute(context);

  update_phase_copy_source();
  apply_next_phase(context);

  anim_graph_player_node_state_machine::current = nullptr;
}

bool anim_graph_player_node_state_machine::update_state(const anim_graph_player_context& context,
                                                        const float pending_phase)
{
  if (_current_node->get_type() == anim_graph_node_type::state) {
    // If we're not transitioning, check if we can start one

    // TODO: if we finish at a breakpoint, it would be great to correct `dt`.
    // E.g. if we had dt to go from 0.48 to 0.52 phase, and we have a breakpoint at 0.5
    // at which we start a transition, only update transition for `dt` -
    // `dt_used_to_move_from_0.48_to_0.5`.

    auto* current_state{polymorphic_downcast<anim_graph_player_node_state*>(_current_node)};
    const auto& transitions{current_state->get_out_transitions()};

    _transition_source_candidate = current_state;

    // First check all breakpoints before current phase

    for (const float breakpoint : current_state->get_breakpoints()) {
      if (breakpoint >= pending_phase) {
        break;
      }

      _transition_source_candidate_phase = breakpoint;

      for (anim_graph_player_node_state_transition* transition : transitions) {
        if (!transition->conditions_are_satisfied(context)) {
          continue;
        }

        _transition_source = current_state;
        _current_node = transition;
        return true;
      }
    }

    // Then finally a pending phase check

    _transition_source_candidate_phase = pending_phase;

    for (anim_graph_player_node_state_transition* transition : transitions) {
      if (!transition->conditions_are_satisfied(context)) {
        continue;
      }

      _transition_source = current_state;
      _current_node = transition;
      return true;
    }
  }
  else {
    // If we're in a transition, check if it's finished and we can switch to destination state

    auto* current_transition{
        polymorphic_downcast<anim_graph_player_node_state_transition*>(_current_node)};
    if (current_transition->is_finished(pending_phase)) {
      _current_node = current_transition->get_current_destination_state_node();
      return true;
    }
  }

  return false;
}

void anim_graph_player_node_state_machine::update_phase_copy_source()
{
  if (_current_node->get_type() == anim_graph_node_type::state_transition) {
    auto* current_transition{
        polymorphic_downcast<anim_graph_player_node_state_transition*>(_current_node)};
    set_phase_copy_source(current_transition->get_current_destination_state_node());
  }
  else {
    set_phase_copy_source(_current_node);
  }
}
}  // namespace eely::internal