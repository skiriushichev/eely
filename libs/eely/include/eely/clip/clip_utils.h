#pragma once

#include "eely/base/assert.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <limits>
#include <vector>

namespace eely::internal {
// Return element of a vector which has data for specified joint index,
// and remember current position. This is to speedup subsequent getters,
// assuming that vector is sorted by joint.
template <typename TVector>
auto& get_by_joint_index(TVector& data, gsl::index& data_index, gsl::index joint_index);

// Implementation

template <typename TVector>
auto& get_by_joint_index(TVector& data, gsl::index& data_index, gsl::index joint_index)
{
  const gsl::index initial_index{data_index};

  while (true) {
    auto& element{data[data_index]};
    if (data[data_index].joint_index == joint_index) {
      return element;
    }

    ++data_index;

    if (data_index == data.size()) {
      data_index = 0;
    }

    EXPECTS(data_index != initial_index);
  }
}
}  // namespace eely::internal