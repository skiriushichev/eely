#include "eely/anim_graph/anim_graph_player_node_clip.h"

#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_clip.h"

#include <any>
#include <memory>

namespace eely::internal {
anim_graph_player_node_clip::anim_graph_player_node_clip(const clip& clip)
    : anim_graph_player_node_pose_base{anim_graph_node_type::clip}, _player{clip.create_player()}
{
  _job_clip.set_player(*_player);
  set_duration_s(_player->get_duration_s());
}

void anim_graph_player_node_clip::compute_impl(const anim_graph_player_context& context,
                                               std::any& out_result)
{
  anim_graph_player_node_pose_base::compute_impl(context, out_result);

  apply_next_phase(context);

  _job_clip.set_time(get_phase() * get_duration_s());
  out_result = context.job_queue.add_job(_job_clip);
}
}  // namespace eely::internal