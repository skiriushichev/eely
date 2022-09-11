#include "eely/anim_graph/fsm/fsm_player_node_transition.h"

#include "eely/anim_graph/fsm/fsm_node_transition.h"
#include "eely/anim_graph/fsm/fsm_player_node_base.h"
#include "eely/base/assert.h"
#include "eely/base/string_id.h"
#include "eely/job/job_queue.h"

#include <optionaL>
#include <utility>
#include <variant>

namespace eely::internal {
fsm_player_node_transition::fsm_player_node_transition()
    : fsm_player_node_base(fsm_player_node_type::transition)
{
}

fsm_player_node_transition::fsm_player_node_transition(transition_trigger_variant trigger,
                                                       transition_blending blending,
                                                       float duration_s)
    : fsm_player_node_base(fsm_player_node_type::transition),
      _trigger{std::move(trigger)},
      _blending{blending}
{
  set_duration_s(duration_s);
}

fsm_player_node_base* fsm_player_node_transition::get_source() const
{
  return _node_source;
}

void fsm_player_node_transition::set_source(fsm_player_node_base* source)
{
  _node_source = source;
}

fsm_player_node_base* fsm_player_node_transition::get_destination() const
{
  return _node_destination;
}

void fsm_player_node_transition::set_destination(fsm_player_node_base* destination)
{
  _node_destination = destination;
}

bool fsm_player_node_transition::should_begin(const anim_graph_context& context)
{
  EXPECTS(_node_source != nullptr);
  EXPECTS(_node_destination != nullptr);

  if (const auto* trigger{std::get_if<transition_trigger_source_end>(&_trigger)}) {
    _node_source->prepare(context);

    // Check if current update would move source state to its end.
    // If it would, we can start this transition.
    const float node_source_phase_delta{context.dt_s / _node_source->get_duration_s()};
    const float node_source_next_phase{_node_source->get_phase() + node_source_phase_delta};
    return node_source_next_phase >= 1.0F;
  }

  if (const auto* trigger{std::get_if<transition_trigger_param_value>(&_trigger)}) {
    return context.params.get(trigger->param_id) == trigger->value;
  }

  EXPECTS(false);
  return false;
}

bool fsm_player_node_transition::is_ended() const
{
  return (get_phase() == 1.0F);
}

void fsm_player_node_transition::prepare(const anim_graph_context& context)
{
  fsm_player_node_base::prepare(context);

  _node_source->prepare(context);
  _node_destination->prepare(context);
}

gsl::index fsm_player_node_transition::enqueue_job(const anim_graph_context& context)
{
  // Transitions do not wrap their phases, needs to stop at 1.0F
  update_phase_from_context(context, false);

  if (_blending == transition_blending::cross_fade) {
    // Play source and destination and parallel and blend their results

    gsl::index source_job{_node_source->enqueue_job(context)};
    gsl::index destination_job{_node_destination->enqueue_job(context)};

    _blend_job.set_first_job_index(source_job);
    _blend_job.set_second_job_index(destination_job);
    _blend_job.set_weight(get_phase());
    return context.job_queue.add_job(_blend_job);
  }

  if (_blending == transition_blending::frozen_fade) {
    // Remember current source pose and blend from it to destination that is being played.
    // To do that we need to play source once, remember its pose,
    // and use it later during the transition.

    if (!_pose_saved) {
      // Receive a slot in a queue in which we will save the pose
      if (!_saved_pose_index.has_value()) {
        _saved_pose_index = context.job_queue.acquire_saved_pose_index();
      }

      // Schedule source job + job to save its results

      anim_graph_context pass_on{context};

      if (const auto* trigger{std::get_if<transition_trigger_source_end>(&_trigger)}) {
        pass_on.sync_phase = 1.0F;
      }
      else {
        // We should not move source from current phase when saving
        pass_on.dt_s = 0.0F;
      }

      gsl::index play_source_job{_node_source->enqueue_job(pass_on)};

      _save_job.set_saved_job_index(play_source_job);
      _save_job.set_saved_pose_index(_saved_pose_index.value());
      context.job_queue.add_job(_save_job);

      _restore_job.set_saved_pose_index(_saved_pose_index.value());

      _pose_saved = true;
    }

    gsl::index source_job{context.job_queue.add_job(_restore_job)};
    gsl::index destination_job{_node_destination->enqueue_job(context)};

    _blend_job.set_first_job_index(source_job);
    _blend_job.set_second_job_index(destination_job);
    _blend_job.set_weight(get_phase());
    return context.job_queue.add_job(_blend_job);
  }

  EXPECTS(false);
  return -1;
}

void fsm_player_node_transition::on_start(const anim_graph_context& context)
{
  fsm_player_node_base::on_start(context);

  _pose_saved = false;
}
}  // namespace eely::internal