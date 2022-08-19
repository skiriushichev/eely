#pragma once

#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <memory>

namespace eely::internal {
// Base interface for a job that produces a pose.
// These jobs are put into a `job_queue` and are executed in order.
// Each job can either create a new pose or make operation on existing ones,
// e.g. blend results of two other jobs.
// Jobs use `skeleton_pose_pool` for all the poses,
// and according methods can be used to transfer or release them.
class job_base {
public:
  // Execute the job and write result pose.
  void execute(const job_queue& queue);

  // Return a read-only result pose.
  // Should only be called after this job is executed.
  const skeleton_pose_pool::ptr& get_result_pose();

  // Transfer ownership of a result pose to another job.
  // Should only be called after this job is executed.
  skeleton_pose_pool::ptr transfer_result_pose();

  // Release a result pose back to the pool.
  // Should only be called after this job is executed.
  void release_result_pose();

private:
  virtual skeleton_pose_pool::ptr execute_impl(const job_queue& queue) = 0;

  skeleton_pose_pool::ptr _result;
};
}  // namespace eely::internal