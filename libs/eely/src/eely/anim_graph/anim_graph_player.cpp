#include "eely/anim_graph/anim_graph_player.h"

namespace eely {
anim_graph_player::anim_graph_player(const btree& btree, const params& params)
    : _player_variant{internal::btree_player{btree}},
      _job_queue{*btree.get_project().get_resource<skeleton>(btree.get_skeleton_id())},
      _context{.job_queue = _job_queue, .params = params}
{
}

anim_graph_player::anim_graph_player(const fsm& fsm, const params& params)
    : _player_variant{internal::fsm_player{fsm}},
      _job_queue{*fsm.get_project().get_resource<skeleton>(fsm.get_skeleton_id())},
      _context{.job_queue = _job_queue, .params = params}
{
}

void anim_graph_player::play(float dt_s, skeleton_pose& out_pose)
{
  using namespace eely::internal;

  _context.dt_s = dt_s;

  if (auto* player{std::get_if<btree_player>(&_player_variant)}) {
    player->prepare(_context);
    player->enqueue_job(_context);
  }
  else if (auto* player{std::get_if<fsm_player>(&_player_variant)}) {
    player->prepare(_context);
    player->enqueue_job(_context);
  }

  _job_queue.execute(out_pose);

  ++_context.play_counter;
}
}  // namespace eely