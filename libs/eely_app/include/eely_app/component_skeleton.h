#pragma once

#include <eely/skeleton.h>
#include <eely/skeleton_pose.h>

namespace eely {
struct component_skeleton final {
  const skeleton* skeleton{nullptr};
  skeleton_pose pose;

  bool render{true};
};
}  // namespace eely