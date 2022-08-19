#pragma once

#include "eely/job/job_base.h"
#include "eely/job/job_queue.h"
#include "eely/skeleton/skeleton_pose.h"
#include "eely/skeleton/skeleton_pose_pool.h"

#include <gsl/util>

#include <memory>

namespace eely::internal {
// Job that adds an additive pose to another.
class job_add final : public job_base {
public:
  struct dependencies final {
    gsl::index first{-1};
    gsl::index second{-1};
  };

  void set_dependencies(const dependencies& dependencies);

private:
  skeleton_pose_pool::ptr execute_impl(const job_queue& queue) override;

  dependencies _dependencies;
};
}  // namespace eely::internal