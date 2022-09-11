#pragma once

#include "eely/clip/clip_player_base.h"
#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <memory>

namespace eely::internal {
// Job that plays a clip at specified time.
class job_play final : public job_base {
public:
  // Set player for the job to use.
  void set_player(clip_player_base& player);

  // Set time to play the clip at.
  void set_time(float time_s);

private:
  skeleton_pose_pool::ptr execute_impl(job_queue& queue) override;

  clip_player_base* _player{nullptr};
  float _time_s{0.0F};
};

// Implementation

inline void job_play::set_player(clip_player_base& player)
{
  _player = &player;
}

inline void job_play::set_time(const float time_s)
{
  EXPECTS(_player != nullptr);
  EXPECTS(time_s >= 0.0F && time_s <= _player->get_duration_s());

  _time_s = time_s;
}

inline skeleton_pose_pool::ptr job_play::execute_impl(job_queue& queue)
{
  auto pose_ptr{queue.get_pose_pool().borrow()};
  _player->play(_time_s, *pose_ptr);
  return pose_ptr;
}
}  // namespace eely::internal