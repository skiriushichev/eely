#include "eely/job/job_queue.h"

#include "eely/base/assert.h"
#include "eely/job/job_base.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <algorithm>
#include <vector>

namespace eely::internal {
job_queue::job_queue(const skeleton& skeleton) : _pose_pool{skeleton} {}

gsl::index job_queue::add_job(job_base& job)
{
  EXPECTS(std::find(_jobs.begin(), _jobs.end(), &job) == _jobs.end());
  _jobs.push_back(&job);
  return std::ssize(_jobs) - 1;
}

skeleton_pose_pool& job_queue::get_pose_pool()
{
  return _pose_pool;
}

gsl::index job_queue::acquire_saved_pose_index()
{
  _saved_poses.emplace_back();
  return std::ssize(_saved_poses) - 1;
}

void job_queue::save_pose(const job_base& job, gsl::index pose_index)
{
  if (_saved_poses[pose_index] == nullptr) {
    _saved_poses[pose_index] = _pose_pool.borrow();
  }

  *_saved_poses[pose_index] = *job.get_result_pose();
}

const skeleton_pose_pool::ptr& job_queue::restore_pose(gsl::index pose_index)
{
  return _saved_poses[pose_index];
}

job_base& job_queue::get_job(gsl::index job_index)
{
  job_base* result{_jobs.at(job_index)};
  EXPECTS(result != nullptr);
  return *result;
}

void job_queue::execute(skeleton_pose& out_pose)
{
  EXPECTS(!_jobs.empty());

  for (job_base* job : _jobs) {
    job->execute(*this);
  }

  job_base* final_job{_jobs.back()};

  out_pose = *final_job->get_result_pose();

  for (job_base* job : _jobs) {
    job->release_result_pose();
  }

  _jobs.clear();
}
}  // namespace eely::internal