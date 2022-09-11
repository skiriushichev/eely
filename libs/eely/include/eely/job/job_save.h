#pragma once

#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <memory>

namespace eely::internal {
// Job that saves result pose to be used later.
// E.g. when doing frozen fade transitions.
// This pose can be then restored using `job_restore`.
class job_save final : public job_base {
public:
  // Set index at which pose should be saved in a queue.
  void set_saved_pose_index(gsl::index index);

  // Set index of a job, whose results needs to be saved.
  void set_saved_job_index(gsl::index index);

private:
  skeleton_pose_pool::ptr execute_impl(job_queue& queue) override;

  std::optional<gsl::index> _pose_index;
  std::optional<gsl::index> _job_index;
};

// Implementation

inline void job_save::set_saved_pose_index(const gsl::index index)
{
  _pose_index = index;
}

inline void job_save::set_saved_job_index(const gsl::index index)
{
  _job_index = index;
}

inline skeleton_pose_pool::ptr job_save::execute_impl(job_queue& queue)
{
  queue.save_pose(queue.get_job(_job_index.value()), _pose_index.value());
  return nullptr;
}
}  // namespace eely::internal