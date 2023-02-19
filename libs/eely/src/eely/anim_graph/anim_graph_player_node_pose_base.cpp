#include "eely/anim_graph/anim_graph_player_node_pose_base.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/base/assert.h"

#include <optional>

namespace eely::internal {
anim_graph_player_node_pose_base::anim_graph_player_node_pose_base(const anim_graph_node_type type,
                                                                   const int id)
    : anim_graph_player_node_base{type, id}
{
}

void anim_graph_player_node_pose_base::update_duration(const anim_graph_player_context& context)
{
  _last_duration_update_play_counter = context.play_counter;
}

float anim_graph_player_node_pose_base::get_next_phase_unwrapped(
    const anim_graph_player_context& context) const
{
  const bool copy_phase{(_phase_rules & phase_rules::copy) != 0};
  if (copy_phase) {
    EXPECTS(_phase_copy_source != nullptr);
    return _phase_copy_source->get_next_phase_unwrapped(context);
  }

  const bool allow_sync{(_phase_rules & phase_rules::sync) != 0};

  EXPECTS(!context.sync_phase.has_value() || context.sync_enabled);
  if (context.sync_enabled && allow_sync) {
    EXPECTS(context.sync_phase.has_value());
    return context.sync_phase.value();
  }

  if (_duration_s == 0.0F) {
    return 1.0F;
  }

  if (is_first_play(context)) {
    return 0.0F;
  }

  const bool reverse_direction{(_phase_rules & phase_rules::reversed) != 0};
  const float direction_sign{reverse_direction ? -1.0F : 1.0F};

  return _phase + direction_sign * context.dt_s / _duration_s;
}

void anim_graph_player_node_pose_base::compute_impl(const anim_graph_player_context& context,
                                                    std::any& out_result)
{
  anim_graph_player_node_base::compute_impl(context, out_result);

  // Call `update_duration` if it wasn't called manually before computation.
  if (_last_duration_update_play_counter != context.play_counter) {
    update_duration(context);
    _last_duration_update_play_counter = context.play_counter;
  }
}

void anim_graph_player_node_pose_base::apply_next_phase(const anim_graph_player_context& context)
{
  const bool copy_phase{(_phase_rules & phase_rules::copy) != 0};
  if (copy_phase) {
    EXPECTS(_phase_copy_source != nullptr);
    _phase = _phase_copy_source->get_phase();
    return;
  }

  const float unwrapped_new_phase{get_next_phase_unwrapped(context)};

  const bool allow_wrap{(_phase_rules & phase_rules::wrap) != 0};

  if (allow_wrap && unwrapped_new_phase > 1.0F) {
    _phase = std::fmod(unwrapped_new_phase, 1.0F);
  }
  else {
    _phase = std::clamp(unwrapped_new_phase, 0.0F, 1.0F);
  }

  EXPECTS(std::isfinite(_phase));
  EXPECTS(_phase >= 0.0F && _phase <= 1.0F);
}
}  // namespace eely::internal