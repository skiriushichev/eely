#include "eely/btree/btree_player_node_clip.h"

#include "eely/btree/btree_player_node_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_base.h"
#include "eely/job/job_play.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely::internal {
btree_player_node_clip::btree_player_node_clip(const clip& clip) : _player{clip.create_player()}
{
  EXPECTS(_player != nullptr);

  _job.set_player(*_player);

  set_duration_s(_player->get_duration_s());
}

gsl::index btree_player_node_clip::play(const context& context)
{
  if (context.sync_phase.has_value()) {
    _player_time_s = _player->get_duration_s() * context.sync_phase.value();
  }
  else {
    _player_time_s = std::fmod(_player_time_s + context.dt_s, _player->get_duration_s());
  }

  EXPECTS(_player_time_s >= 0.0F && _player_time_s <= _player->get_duration_s());

  _job.set_time(_player_time_s);

  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal