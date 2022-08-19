#pragma once

#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <vector>

namespace eely::internal {
class job_base;

// Job queue list jobs to be executed to produce final pose for specified skeleton.
// Animation graphs and blend trees produce these jobs when traversed.
class job_queue final {
public:
  // Construct job queue for specified skeleton.
  explicit job_queue(const skeleton& skeleton);

  // Add job to be executed and return its index.
  gsl::index add_job(job_base& job);

  // Execute the queue and write results into `out_pose`.
  void execute(skeleton_pose& out_pose);

  // Get job by its index.
  [[nodiscard]] job_base& get_job(gsl::index job_index) const;

  // Get pose pool to be used during execution.
  [[nodiscard]] skeleton_pose_pool& get_pose_pool() const;

private:
  std::vector<job_base*> _jobs;

  // Pool is mutable because jobs can access the queue when they're invoked,
  // and they need the pool to acquire poses.
  // We don't want them to change anything in the queue itself, so it is passed as const.
  mutable skeleton_pose_pool _pose_pool;
};
}  // namespace eely::internal