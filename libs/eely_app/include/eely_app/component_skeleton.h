#pragma once

#include <eely/math/float4.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_pose.h>

#include <unordered_map>

namespace eely {
struct component_skeleton final {
  enum joint_render_flags {
    none = 0,
    frame = 1 << 0,
    constraint_parent_frame = 1 << 1,
    constraint_child_frame = 1 << 2,
    constraint_limits = 1 << 3,
  };

  const skeleton* skeleton{nullptr};
  skeleton_pose pose;

  bool pose_render{true};
  float4 pose_render_color{0.8F, 0.8F, 0.8F, 0.8F};

  std::unordered_map<string_id, joint_render_flags> joint_renders;
};
}  // namespace eely