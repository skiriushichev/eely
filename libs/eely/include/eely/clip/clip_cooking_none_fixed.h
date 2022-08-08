#pragma once

#include "eely/base/assert.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/math/float3.h"
#include "eely/math/float4.h"
#include "eely/math/math_utils.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/skeleton/skeleton.h"

#include <limits>
#include <optional>
#include <vector>

// Helper methods for cooking clips with
// `clip_compression_scheme::none` and `clip_compression_scheme::fixed`.

namespace eely::internal {
// Flags for a key inside a clip data.
enum compression_key_flags {
  has_joint_index = 1 << 0,
  has_time = 1 << 1,
  has_translation = 1 << 2,
  has_rotation = 1 << 3,
  has_scale = 1 << 4,
};

static constexpr gsl::index bits_compression_key_flags = 5;

// Key to be written in a buffer.
struct cooked_key final {
  // Joint index is not `std::optional` because for compressed data we need index
  // even though it didn't change from previous key, to get joint's metadata.
  // `joint_index_changed` indicates if joint is actually different from previous key.
  gsl::index joint_index{std::numeric_limits<gsl::index>::max()};
  bool joint_index_changed{false};

  std::optional<float> time_s;

  std::optional<float3> translation;
  std::optional<quaternion> rotation;
  std::optional<float3> scale;
};

// Precision data for `remove_rest_pose_tracks`.
struct remove_rest_pose_tracks_precision final {
  float rotation_rad{deg_to_rad(0.001F)};
  float translation{0.001F};
  float scale{0.001F};
};

// Cook tracks into a single buffer, using provided writer.
template <typename TWriter>
requires std::invocable<TWriter, const cooked_key&>
void clip_cook(const std::vector<clip_uncooked::track>& tracks,
               const skeleton& skeleton,
               TWriter& writer);

// Remove tracks that match rest pose with specified precision.
std::vector<clip_uncooked::track> remove_rest_pose_tracks(
    const std::vector<clip_uncooked::track>& tracks,
    const skeleton& skeleton,
    const remove_rest_pose_tracks_precision& precision = remove_rest_pose_tracks_precision{});

// Implementation

// Helper structure for cooking that holds transform's component
// with some auxiliary data needed for sorting.
struct key_component final {
  gsl::index joint_index{std::numeric_limits<gsl::index>::max()};
  float time{-1.0F};
  float4 data;  // 4 floats for quaternion, 3 floats for scale and translation
  transform_components component{transform_components::translation};

  // "Time to use" is time when we need this key when playing animation.
  // This is not the same as time of a key, since we interpolate between keys.
  // E.g. if we have two keys for translation, first is on t=0.0 and second is on 6.0,
  // time to use for both keys will be 0.0, since we need them for any t in [0.0, 6.0] interval.
  // If there is a third key at t=7.0, its time to use is equal to 6th second,
  // since we need it for any t in [6.0, 7.0] interval.
  // So "time to use" = 0 if this is a first key, otherwise it is equal to previous key's time.
  float time_to_use{-1.0F};
};

bool key_component_compare(const key_component& a, const key_component& b);

std::vector<key_component> key_component_collect(const std::vector<clip_uncooked::track>& tracks,
                                                 const skeleton& skeleton);

template <typename TWriter>
requires std::invocable<TWriter, const cooked_key&>
void clip_cook(const std::vector<clip_uncooked::track>& tracks,
               const skeleton& skeleton,
               TWriter& writer)
{
  // Cooking puts all tracks in a single buffer as a collection of keys.
  // Each key has:
  //   - Flags, that say what data this key holds
  //   - Index of a joint, if it is changed from previous key
  //   - Key time, if it is changed from previous key
  //   - Joint transform components (can have full transform or a subset, e.g rotation and scale)
  //
  // All keys are sorted in a way that allows traverse this buffer
  // sequentially when animation is played to reduce cache misses:
  //   - First two keys are always in the beginning, since we need second key for any t > 0.0F
  //     (assuming first key is at 0.0 time)
  //   - Then, keys are sorted by joint index
  //     (to traverse joints in ascending order, which is how it is stored in a skeleton)
  //   - Then, by time and transform's component type (e.g. first translations, then rotations)
  // This sorting ensures that once we hit a key that is not needed,
  // all keys that follow are not yet needed as well.

  std::vector<key_component> key_components{key_component_collect(tracks, skeleton)};

  std::optional<float> previous_time;
  std::optional<float> previous_time_to_use;
  std::optional<gsl::index> previous_joint_index;

  cooked_key key_to_write;

  const size_t key_components_size{key_components.size()};
  EXPECTS(key_components_size > 0);
  for (gsl::index i{0}; i < key_components_size; ++i) {
    const key_component& key_ext{key_components[i]};

    // Write data whenever we moved to a new time or a new index

    const bool time_changed{previous_time.has_value() && key_ext.time != previous_time.value()};
    const bool time_to_use_changed{previous_time_to_use.has_value() &&
                                   key_ext.time_to_use != previous_time_to_use.value()};
    const bool joint_changed{previous_joint_index.has_value() &&
                             key_ext.joint_index != previous_joint_index.value()};
    const bool flush{time_changed || time_to_use_changed || joint_changed};

    if (flush) {
      writer(key_to_write);
      key_to_write = {};
    }

    if (!previous_time.has_value() || key_ext.time != previous_time.value()) {
      key_to_write.time_s = key_ext.time;
    }

    key_to_write.joint_index = key_ext.joint_index;
    if (!previous_joint_index.has_value() || key_ext.joint_index != previous_joint_index.value()) {
      key_to_write.joint_index_changed = true;
    }

    switch (key_ext.component) {
      case transform_components::translation: {
        key_to_write.translation = float3{key_ext.data.x, key_ext.data.y, key_ext.data.z};
      } break;

      case transform_components::rotation: {
        key_to_write.rotation =
            quaternion{key_ext.data.x, key_ext.data.y, key_ext.data.z, key_ext.data.w};
      } break;

      case transform_components::scale: {
        key_to_write.scale = float3{key_ext.data.x, key_ext.data.y, key_ext.data.z};
      } break;
    }

    previous_time = key_ext.time;
    previous_time_to_use = key_ext.time_to_use;
    previous_joint_index = key_ext.joint_index;

    // If this is a last key, write as well
    if (i == key_components_size - 1) {
      writer(key_to_write);
    }
  }
}
}  // namespace eely::internal