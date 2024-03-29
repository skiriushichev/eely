#pragma once

#include "eely/base/assert.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <concepts>
#include <limits>
#include <vector>

// Utility functions for cursors that are used to play clips
// with `clip_compression_scheme::none` and `clip_compression_scheme::fixed`.

namespace eely::internal {
// Cursor into animation data for a specific transform component.
// Each component is described by up to two points,
// and result value is interpolation between them based on time.
template <typename T>
requires std::same_as<T, float3> || std::same_as<T, quaternion>
struct cursor_component final {
  // Index of a joint this cursor is for.
  gsl::index joint_index{std::numeric_limits<gsl::index>::max()};

  // Time for a left component.
  // If negative, there is no left component.
  float left_time_s{-1.0F};

  // Time for a right component.
  // If negative, there is no right component.
  float right_time_s{-1.0F};

  // Left transform component.
  T left;

  // Right transform component.
  T right;
};

// Cursor into animation data.
// Cursor remembers data from previous update to speed up the next one.
struct cursor final {
  // Last time clip was played for.
  float last_play_time_s{-1.0F};

  // Last position in data buffer.
  gsl::index last_data_pos{0};

  // Last time read.
  float last_data_time_s{-1.0F};

  // Last joint read.
  gsl::index last_data_joint_index{std::numeric_limits<gsl::index>::max()};

  // Component cursors for joint translations.
  std::vector<cursor_component<float3>> translations;

  // Component cursors for joint rotations.
  std::vector<cursor_component<quaternion>> rotations;

  // Component cursors for joint scales.
  std::vector<cursor_component<float3>> scales;

  // Shallow joint index for an animation,
  // i.e. first index of a joint that is changed in an animation.
  // Used for optimizing skeleton pose object transform calculations.
  gsl::index shallow_joint_index{std::numeric_limits<gsl::index>::max()};
};

// Describes what transform components are changed in an animation for specific joint.
struct joint_components final {
  gsl::index joint_index{std::numeric_limits<gsl::index>::max()};
  int components{0};
};

// Reset cursor to default state, removing its left and right values.
template <typename T>
void cursor_component_reset(cursor_component<T>& cursor_component);

// Return `true` if cursor's data is outdated for specified time,
// assuming there is still data to be used.
// E.g. either it has no value or value is too old.
template <typename T>
[[nodiscard]] bool cursor_component_is_outdated(const cursor_component<T>& cursor_component,
                                                float time_s);

// Advance to the next value.
// Old right moves to the left, and next value becomes the new right.
template <typename T>
void cursor_component_advance(cursor_component<T>& cursor_component,
                              const T& next_value,
                              float next_time_s);

// Calculate value for specified time.
template <typename T>
[[nodiscard]] T cursor_component_calculate(const cursor_component<T>& cursor_component,
                                           float time_s);

// Prepare cursor for playing tracks with specified joint components.
void cursor_init(cursor& cursor, const std::vector<joint_components>& joints_components);

// Reset cursor to default state.
void cursor_reset(cursor& cursor);

// Calculate skeleton pose based on current cursor state.
void cursor_calculate_pose(const cursor& cursor, float time_s, skeleton_pose& out_pose);

// Collect joints components participating in a clip.
void joint_components_collect(const std::vector<clip_uncooked_track>& tracks,
                              const skeleton& skeleton,
                              std::vector<joint_components>& out_joints_components);

// Implementation

template <typename T>
void cursor_component_reset(cursor_component<T>& cursor_component)
{
  cursor_component.left_time_s = -1.0F;
  cursor_component.right_time_s = -1.0F;
}

template <typename T>
bool cursor_component_is_outdated(const cursor_component<T>& cursor_component, const float time_s)
{
  // Outdated when we either have no left key,
  // or we have both keys but they're too old.
  return cursor_component.left_time_s < 0.0F || cursor_component.right_time_s < time_s;
}

template <typename T>
void cursor_component_advance(cursor_component<T>& cursor_component,
                              const T& next_value,
                              const float next_time_s)
{
  cursor_component.left = cursor_component.right;
  cursor_component.left_time_s = cursor_component.right_time_s;

  cursor_component.right = next_value;
  cursor_component.right_time_s = next_time_s;
}

template <typename T>
T cursor_component_calculate(const cursor_component<T>& cursor_component, const float time_s)
{
  if (cursor_component.left_time_s < 0.0F) {
    // A single key
    EXPECTS(cursor_component.right_time_s >= 0.0F);
    return cursor_component.right;
  }

  EXPECTS(cursor_component.left_time_s >= 0.0F);
  EXPECTS(cursor_component.right_time_s >= 0.0F);

  const float duration{cursor_component.right_time_s - cursor_component.left_time_s};
  EXPECTS(duration > 0.0F);

  // Coeff can be negative if time requested is smaller than left time,
  // and can be higher than one if time is greater than right one
  const float coeff{(time_s - cursor_component.left_time_s) / duration};

  if (coeff <= 0.0F) {
    return cursor_component.left;
  }

  if (coeff >= 1.0F) {
    return cursor_component.right;
  }

  if constexpr (std::is_same_v<T, float3>) {
    return float3_lerp(cursor_component.left, cursor_component.right, coeff);
  }

  if constexpr (std::is_same_v<T, quaternion>) {
    return quaternion_slerp(cursor_component.left, cursor_component.right, coeff);
  }
}
}  // namespace eely::internal