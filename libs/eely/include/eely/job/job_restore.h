#pragma once

#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <memory>

namespace eely::internal {
// Job that restores previously saved pose.
class job_restore final : public job_base {
public:
  void set_saved_pose_index(gsl::index index);

private:
  skeleton_pose_pool::ptr execute_impl(job_queue& queue) override;

  std::optional<gsl::index> _pose_index;
};

// Implementation

inline void job_restore::set_saved_pose_index(const gsl::index index)
{
  _pose_index = index;
}

inline skeleton_pose_pool::ptr job_restore::execute_impl(job_queue& queue)
{
  const skeleton_pose_pool::ptr& restored_pose{queue.restore_pose(_pose_index.value())};
  EXPECTS(restored_pose != nullptr);

  skeleton_pose_pool::ptr pose_ptr{queue.get_pose_pool().borrow()};
  *pose_ptr = *restored_pose;
  return pose_ptr;
}
}  // namespace eely::internal