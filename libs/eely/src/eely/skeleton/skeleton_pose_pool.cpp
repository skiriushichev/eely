#include "eely/skeleton/skeleton_pose_pool.h"

#include "eely/base/assert.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <algorithm>
#include <memory>
#include <vector>

namespace eely::internal {
skeleton_pose_pool::deleter::deleter(skeleton_pose_pool& pool) : _pool{&pool} {}

void skeleton_pose_pool::deleter::operator()(skeleton_pose* ptr)
{
  EXPECTS(ptr != nullptr);

#if defined(EELY_DEBUG)
  EXPECTS(_pool->_borrows > 0);
  --_pool->_borrows;
#endif

  EXPECTS(std::find_if(_pool->_poses.begin(), _pool->_poses.end(), [ptr](auto& pool_pose) {
            return pool_pose.get() == ptr;
          }) == _pool->_poses.end());
  _pool->_poses.push_back(std::unique_ptr<skeleton_pose>{ptr});
}

skeleton_pose_pool::skeleton_pose_pool(const skeleton& skeleton) : _skeleton{skeleton} {}

skeleton_pose_pool::~skeleton_pose_pool()
{
  EXPECTS(_borrows == 0);
}

skeleton_pose_pool::ptr skeleton_pose_pool::borrow()
{
#if defined(EELY_DEBUG)
  EXPECTS(_borrows >= 0);
  ++_borrows;
#endif

  if (_poses.empty()) {
    return ptr{new skeleton_pose{_skeleton}, deleter{*this}};
  }

  skeleton_pose* ptr_to_borrow{_poses.back().release()};
  _poses.pop_back();

  ptr result{ptr_to_borrow, deleter{*this}};
  return result;
}
}  // namespace eely::internal