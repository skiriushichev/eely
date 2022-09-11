#include "eely/anim_graph/btree/btree_player_node_clip.h"

#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_play.h"
#include "eely/job/job_queue.h"

#include <gsl/util>

#include <memory>
#include <optional>

namespace eely::internal {
btree_player_node_clip::btree_player_node_clip(const clip& clip) : _player{clip.create_player()}
{
  EXPECTS(_player != nullptr);

  set_duration_s(_player->get_duration_s());

  _job.set_player(*_player);
}

gsl::index btree_player_node_clip::enqueue_job(const anim_graph_context& context)
{
  update_phase_from_context(context);

  _job.set_time(get_phase() * get_duration_s());
  return context.job_queue.add_job(_job);
}
}  // namespace eely::internal