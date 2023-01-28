#include "eely/anim_graph/anim_graph_player_node_state_transition.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_node_state_transition.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_state.h"
#include "eely/anim_graph/anim_graph_player_node_state_machine.h"
#include "eely/job/job_blend.h"
#include "eely/job/job_restore.h"
#include "eely/job/job_save.h"

#include <any>
#include <array>
#include <cmath>

namespace eely::internal {
anim_graph_player_node_state_transition::anim_graph_player_node_state_transition(
    const transition_type type,
    const float duration_s)
    : anim_graph_player_node_pose_base{anim_graph_node_type::state_transition}, _type{type}
{
  // Transition does not apply synchronized phase on purpose since:
  //  - it can move backwards (when reversed)
  //  - it is a relatively short period of time only needed to smooth out switches between states
  //  - states being played during transition do apply synchronizations
  //
  // Transition does not apply wrapping phase because it doesn't make sense:
  // it is finished exactly at 1.0F (or 0.0F if reversed), and should never move further.
  set_phase_rules(0);

  set_duration_s(duration_s);
}

void anim_graph_player_node_state_transition::update_duration(
    const anim_graph_player_context& context)
{
  anim_graph_player_node_pose_base::update_duration(context);

  // Update reversed status as well as saved poses slots.
  // We have two slots for saved poses:
  // one is to save reuslts of a transition, and another is to blend from.
  // Once reversed status changed, they switch their purposes,
  // i.e. we start blending from last saved transition pose
  // and begin saving new reuslts in another slot

  const bool can_continue{conditions_are_satisfied(context)};

  if (is_first_play(context)) {
    EXPECTS(can_continue);

    _reversed = false;

    _saved_pose_source_slot_index = 0;
    _saved_pose_source_phase = 0.0F;
  }

  const bool switch_reversed_status{(_reversed && can_continue) || (!_reversed && !can_continue)};

  if (switch_reversed_status) {
    _reversed = !_reversed;

    _saved_pose_source_slot_index = (_saved_pose_source_slot_index == 0) ? 1 : 0;
    _saved_pose_source_phase = get_phase();

    // TODO: when we reverse transition in any direction,
    // the state we're transitioning to will be reset to zero phase.
    // Is this a problem? Should we freeze its phase? Should we move its phase?
  }

  set_phase_rule(phase_rules::reversed, _reversed);

  // Update current source and destination states and update destination's duration

  const auto* const state_machine = anim_graph_player_node_state_machine::get_current();
  EXPECTS(state_machine != nullptr);

  if (_reversed) {
    _current_source = _destination_state_node;
    _current_destination = state_machine->get_transition_source();
  }
  else {
    _current_source = state_machine->get_transition_source();
    _current_destination = _destination_state_node;
  }

  _current_destination->update_duration(context);
}

void anim_graph_player_node_state_transition::collect_descendants(
    std::vector<const anim_graph_player_node_base*>& out_descendants) const
{
  if (_condition_node != nullptr) {
    out_descendants.push_back(_condition_node);
    _condition_node->collect_descendants(out_descendants);
  }

  if (_destination_state_node != nullptr) {
    out_descendants.push_back(_destination_state_node);
    _destination_state_node->collect_descendants(out_descendants);
  }
}

bool anim_graph_player_node_state_transition::conditions_are_satisfied(
    const anim_graph_player_context& context) const
{
  return std::any_cast<bool>(_condition_node->compute(context));
}

bool anim_graph_player_node_state_transition::is_finished(const float phase) const
{
  if (_reversed) {
    return phase <= 0.0F;
  }

  return phase >= 1.0F;
}

const anim_graph_player_node_base* anim_graph_player_node_state_transition::get_condition_node()
    const
{
  return _condition_node;
}

void anim_graph_player_node_state_transition::set_condition_node(
    anim_graph_player_node_base* const node)
{
  _condition_node = node;
}

anim_graph_player_node_state*
anim_graph_player_node_state_transition::get_original_destination_state_node() const
{
  return _destination_state_node;
}

void anim_graph_player_node_state_transition::set_original_destination_state_node(
    anim_graph_player_node_state* node)
{
  _destination_state_node = node;
}

anim_graph_player_node_state*
anim_graph_player_node_state_transition::get_current_destination_state_node()
{
  return _current_destination;
}

void anim_graph_player_node_state_transition::compute_impl(const anim_graph_player_context& context,
                                                           std::any& out_result)
{
  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  apply_next_phase(context);

  // Acquire pose indices

  if (!_saved_pose_slots_acquired) {
    _saved_pose_slots_acquired = true;
    for (gsl::index i{0}; i < std::ssize(_saved_pose_slots); ++i) {
      _saved_pose_slots.at(i) = context.job_queue.acquire_saved_pose_slot();
    }
  }

  // Play transition

  const gsl::index saved_pose_source_slot{_saved_pose_slots.at(_saved_pose_source_slot_index)};
  const gsl::index saved_pose_transition_slot{
      _saved_pose_source_slot_index == 0 ? _saved_pose_slots[1] : _saved_pose_slots[0]};

  const auto* const state_machine = anim_graph_player_node_state_machine::get_current();
  EXPECTS(state_machine != nullptr);

  if (is_first_play(context)) {
    // Play source state on phase we started the transition at
    // Remember result pose and use it during the transition for blending

    anim_graph_player_context context_pass_on{context};

    // Candidate phase is an unwrapped phase, thus we need to wrap it here by ourselves
    context_pass_on.sync_enabled = true;
    context_pass_on.sync_phase =
        std::clamp(state_machine->get_transition_source_candidate_phase(), 0.0F, 1.0F);

    const auto source_job{std::any_cast<gsl::index>(_current_source->compute(context_pass_on))};

    _save_source_state_job.set_saved_job_index(source_job);
    _save_source_state_job.set_saved_pose_index(saved_pose_source_slot);
    context.job_queue.add_job(_save_source_state_job);
  }

  // Restore saved pose (used as a blend source)

  _restore_job.set_saved_pose_index(saved_pose_source_slot);
  const gsl::index restore_job_index{context.job_queue.add_job(_restore_job)};

  // Compute destination pose (used as a blend destination)

  const gsl::index destination_job_index{
      std::any_cast<gsl::index>(_current_destination->compute(context))};

  // Setup blending job

  _blend_job.set_first_job_index(restore_job_index);
  _blend_job.set_second_job_index(destination_job_index);

  const float blend_phase_current{get_phase()};
  const float blend_phase_from{_saved_pose_source_phase};
  const float blend_phase_duration{_reversed ? _saved_pose_source_phase
                                             : 1.0F - _saved_pose_source_phase};
  if (blend_phase_duration == 0.0F) {
    _blend_job.set_weight(1.0F);
  }
  else {
    _blend_job.set_weight(std::abs(blend_phase_current - blend_phase_from) / blend_phase_duration);
  }

  const gsl::index blend_job_index{context.job_queue.add_job(_blend_job)};

  // Remember transition pose resulted from this update

  _save_transition_job.set_saved_job_index(blend_job_index);
  _save_transition_job.set_saved_pose_index(saved_pose_transition_slot);
  context.job_queue.add_job(_save_transition_job);

  out_result = blend_job_index;
}
}  // namespace eely::internal