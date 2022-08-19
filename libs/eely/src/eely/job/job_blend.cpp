#include "eely/job/job_blend.h"

#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <memory>

namespace eely::internal {
void job_blend::set_dependencies(const dependencies& dependencies)
{
  _dependencies = dependencies;
}

skeleton_pose_pool::ptr job_blend::execute_impl(const job_queue& queue)
{
  job_base& first_job{queue.get_job(_dependencies.first)};
  job_base& second_job{queue.get_job(_dependencies.second)};

  // We can reuse one pose from the pool here,
  // let it be a pose from the first job.
  // Second one can be released after the blending, no longer needed.

  skeleton_pose_pool::ptr p0{first_job.transfer_result_pose()};
  const skeleton_pose_pool::ptr& p1{second_job.get_result_pose()};

  skeleton_pose_blend(*p0, *p1, _dependencies.weight, *p0);

  second_job.release_result_pose();

  return p0;
}
}  // namespace eely::internal