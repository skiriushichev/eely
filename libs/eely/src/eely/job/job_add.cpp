#include "eely/job/job_add.h"

#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <memory>

namespace eely::internal {
void job_add::set_dependencies(const dependencies& dependencies)
{
  _dependencies = dependencies;
}

skeleton_pose_pool::ptr job_add::execute_impl(const job_queue& queue)
{
  job_base& first_job{queue.get_job(_dependencies.first)};
  job_base& second_job{queue.get_job(_dependencies.second)};

  // We can reuse one pose from the pool here,
  // let it be a pose from the first job.
  // Second one can be released after the addition, no longer needed.

  skeleton_pose_pool::ptr p0{first_job.transfer_result_pose()};
  const skeleton_pose_pool::ptr& p1{second_job.get_result_pose()};

  skeleton_pose_add(*p0, *p1, *p0);

  second_job.release_result_pose();

  return p0;
}
}  // namespace eely::internal