#pragma once

#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <memory>
#include <vector>

namespace eely::internal {
// Pool for skeleton poses constructed for specific skeleton,
// used when executing job queue.
class skeleton_pose_pool final {
public:
  // Deleter for `unique_ptr` that returns pose back to the pool it was taken from.
  // Should never outlive the pool.
  struct deleter final {
    explicit deleter() = default;
    explicit deleter(skeleton_pose_pool& pool);

    void operator()(skeleton_pose* ptr);

  private:
    skeleton_pose_pool* _pool{nullptr};
  };

  using ptr = std::unique_ptr<skeleton_pose, deleter>;

  explicit skeleton_pose_pool(const skeleton& skeleton);

  skeleton_pose_pool(const skeleton_pose_pool&) = delete;
  skeleton_pose_pool(skeleton_pose_pool&&) = delete;

  ~skeleton_pose_pool();

  skeleton_pose_pool& operator=(const skeleton_pose_pool&) = delete;
  skeleton_pose_pool& operator=(skeleton_pose_pool&&) = delete;

  ptr borrow();

private:
  // For returning back poses into the pool once unique ptr is destroyed
  friend struct deleter;

  const skeleton& _skeleton;
  std::vector<std::unique_ptr<skeleton_pose>> _poses;

#if defined(EELY_DEBUG)
  // Number of borrowed poses that are not yet returned to the pool
  // To check that all poses are returned to the pool before pool is destroyed
  gsl::index _borrows{0};
#endif
};
}  // namespace eely::internal