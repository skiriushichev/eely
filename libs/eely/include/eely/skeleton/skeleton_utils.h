#pragma once

#include <gsl/util>

namespace eely::internal {
// Maximum number of joints in a skeleton.
// To fit in 11 bits + 2047 is reserved as invalid index.
static constexpr gsl::index joints_max_count{2047};

static constexpr gsl::index bits_joints_count{11};
}  // namespace eely::internal