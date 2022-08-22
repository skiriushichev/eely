#pragma once

#include "eely/base/assert.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/project/project.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <limits>
#include <vector>

namespace eely::internal {
// Information about what time range and with which rate is sampled from a clip.
struct clip_sampling_info final {
  float time_from_s{0.0F};
  float time_to_s{0.0F};
  gsl::index rate{0};
};

// Return `true` if a key has a value for specified component.
// Helper for template methods.
template <transform_components TComponent>
bool clip_uncooked_key_has_component(const clip_uncooked_key& key);

// Return value of a specified component in a key,
// Helper for template methods.
template <transform_components TComponent>
auto& clip_uncooked_key_get_component(const clip_uncooked_key& key);

// Return element of a vector which has data for specified joint index,
// and remember current position. This is to speedup subsequent getters,
// assuming that vector is sorted by joint.
template <typename TVector>
auto& get_by_joint_index(TVector& data, gsl::index& data_index, gsl::index joint_index);

// Find key that has specified component and matches a predicate.
template <transform_components TComponent, typename TIter, typename TPredicate>
TIter find_key_component(const TIter& begin, const TIter& end, const TPredicate& predicate);

// Calculate number of samples produced with specified time range and rate.
gsl::index clip_sampling_info_calculate_samples(const clip_sampling_info& info);

// Sample component's value in a track for specified time.
// If there are no keys for this component, default value will be returned.
template <transform_components TComponent>
auto clip_sample_component(const clip_uncooked_track& track,
                           const auto& default_value,
                           float time_s);

// Calculate transforms for a track's joint.
// Values for components that are not present will taken from `default_value`.
void clip_sample_track(const clip_uncooked_track& track,
                       const transform& default_value,
                       const clip_sampling_info& sampling_info,
                       std::vector<transform>& out_samples);

// Calculate tracks for an additive clip.
void clip_calculate_additive_tracks(const project_uncooked& project,
                                    const clip_additive_uncooked& uncooked,
                                    float& out_duration,
                                    std::vector<clip_uncooked_track>& out_tracks);

// Implementation

template <transform_components TComponent>
bool clip_uncooked_key_has_component(const clip_uncooked_key& key)
{
  if constexpr (TComponent == transform_components::translation) {
    return key.translation.has_value();
  }
  else if constexpr (TComponent == transform_components::rotation) {
    return key.rotation.has_value();
  }
  else {
    return key.scale.has_value();
  }
}

template <transform_components TComponent>
auto& clip_uncooked_key_get_component(const clip_uncooked_key& key)
{
  if constexpr (TComponent == transform_components::translation) {
    return key.translation.value();
  }
  else if constexpr (TComponent == transform_components::rotation) {
    return key.rotation.value();
  }
  else {
    return key.scale.value();
  }
}

template <typename TVector>
auto& get_by_joint_index(TVector& data, gsl::index& data_index, gsl::index joint_index)
{
  const gsl::index data_size{std::ssize(data)};

  const gsl::index initial_index{data_index};

  while (true) {
    auto& element{data[data_index]};
    if (element.joint_index == joint_index) {
      return element;
    }

    ++data_index;

    if (data_index == data_size) {
      data_index = 0;
    }

    EXPECTS(data_index != initial_index);
  }
}

template <transform_components TComponent, typename TIter, typename TPredicate>
TIter find_key_component(const TIter& begin, const TIter& end, const TPredicate& predicate)
{
  auto find_iter{std::find_if(begin, end, [&predicate](const auto& kvp) {
    return clip_uncooked_key_has_component<TComponent>(kvp.second) && predicate(kvp);
  })};

  return find_iter;
}

// Calculate component's value in a track for specified time.
template <transform_components TComponent>
auto clip_sample_component(const clip_uncooked_track& track,
                           const auto& default_value,
                           const float time_s)
{
  // Track can have 0, 1 or >1 keys for this component
  // Seach for a key that has this component,
  // and there isn't one, this component will have rest pose value outputed.
  // If there is one, check if is another to lerp between.

  auto key_left_iter{
      std::find_if(track.keys.rbegin(), track.keys.rend(), [time_s](const auto& kvp) {
        return clip_uncooked_key_has_component<TComponent>(kvp.second) && kvp.first <= time_s;
      })};

  if (key_left_iter == track.keys.rend()) {
    // No keys, output rest pose value
    return default_value;
  }

  auto key_right_iter{std::find_if(key_left_iter.base(), track.keys.end(), [](const auto& k) {
    return clip_uncooked_key_has_component<TComponent>(k.second);
  })};

  if (key_right_iter == track.keys.end()) {
    // Only one key, output constant value
    const clip_uncooked_key& key{key_left_iter->second};
    return clip_uncooked_key_get_component<TComponent>(key);
  }

  // Both keys, output lerped value

  const clip_uncooked_key& key_left{key_left_iter->second};
  const clip_uncooked_key& key_right{key_right_iter->second};

  const float interpolation_coeff{(time_s - key_left_iter->first) /
                                  (key_right_iter->first - key_left_iter->first)};

  if constexpr (TComponent == transform_components::translation) {
    return float3_lerp(key_left.translation.value(), key_right.translation.value(),
                       interpolation_coeff);
  }

  if constexpr (TComponent == transform_components::rotation) {
    return quaternion_slerp(key_left.rotation.value(), key_right.rotation.value(),
                            interpolation_coeff);
  }

  if constexpr (TComponent == transform_components::scale) {
    return float3_lerp(key_left.scale.value(), key_right.scale.value(), interpolation_coeff);
  }
}
}  // namespace eely::internal