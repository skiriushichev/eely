#include "eely/job/job_base.h"

#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <memory>

namespace eely::internal {
void job_base::execute(const job_queue& queue)
{
  _result = execute_impl(queue);
}

const skeleton_pose_pool::ptr& job_base::get_result_pose() const
{
  return _result;
}

skeleton_pose_pool::ptr job_base::transfer_result_pose()
{
  return std::move(_result);
}

void job_base::release_result_pose()
{
  _result.reset();
}
}  // namespace eely::internal