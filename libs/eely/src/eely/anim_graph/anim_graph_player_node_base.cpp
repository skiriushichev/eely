#include "eely/anim_graph/anim_graph_player_node_base.h"

#include <cmath>
#include <optional>

namespace eely::internal {
void anim_graph_player_node_base::prepare(const anim_graph_context& context)
{
  if (context.play_counter != _prev_graph_play_counter) {
    if (context.play_counter - _prev_graph_play_counter > 1) {
      on_start(context);
    }

    _prev_graph_play_counter = context.play_counter;
  }
}

void anim_graph_player_node_base::update_phase_from_context(const anim_graph_context& context,
                                                            const bool allow_wrap)
{
  if (context.sync_phase.has_value()) {
    _phase = context.sync_phase.value();
    return;
  }

  if (_duration_s == 0.0F) {
    _phase = 1.0F;
    return;
  }

  const float unwrapped_new_phase{_phase + context.dt_s / _duration_s};
  if (allow_wrap) {
    _phase = std::fmod(unwrapped_new_phase, 1.0F);
  }
  else {
    _phase = std::min(unwrapped_new_phase, 1.0F);
  }
}

void anim_graph_player_node_base::on_start(const anim_graph_context& /*context*/)
{
  set_phase(0.0F);
}
}  // namespace eely::internal