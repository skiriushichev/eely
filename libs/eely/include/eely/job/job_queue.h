#pragma once

#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <vector>

namespace eely::internal {
class job_base;

// Job queue list jobs to be executed to produce final pose for specified skeleton.
// Animation graphs and blendtrees produce these jobs when traversed.
class job_queue final {
public:
  // Construct job queue for specified skeleton.
  explicit job_queue(const skeleton& skeleton);

  // Add job to be executed and return its index.
  gsl::index add_job(job_base& job);

  // Execute the queue and write results into `out_pose`.
  void execute(skeleton_pose& out_pose);

  // Get job by its index.
  [[nodiscard]] job_base& get_job(gsl::index job_index);

  // Get pose pool to be used during execution.
  [[nodiscard]] skeleton_pose_pool& get_pose_pool();

  [[nodiscard]] gsl::index acquire_saved_pose_index();

  void save_pose(const job_base& job, gsl::index pose_index);

  const skeleton_pose_pool::ptr& restore_pose(gsl::index pose_index);

private:
  std::vector<job_base*> _jobs;
  skeleton_pose_pool _pose_pool;
  std::vector<skeleton_pose_pool::ptr> _saved_poses;
};
}  // namespace eely::internal