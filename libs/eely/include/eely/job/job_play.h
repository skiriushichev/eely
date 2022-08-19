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
  skeleton_pose_pool::ptr execute_impl(const job_queue& queue) override;

  clip_player_base* _player{nullptr};
  float _time_s{0.0F};
};
}  // namespace eely::internal