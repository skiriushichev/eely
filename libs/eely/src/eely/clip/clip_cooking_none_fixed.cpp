#include "eely/clip/clip_cooking_none_fixed.h"

#include "eely/base/assert.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/clip/clip_utils.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/skeleton/skeleton.h"

#include <gsl/util>

#include <optional>
#include <tuple>
#include <vector>

namespace eely::internal {
//
template <transform_components TComponent>
static auto linear_key_reduction_component(
    const std::map<float, clip_uncooked_key>& keys,
    const std::map<float, clip_uncooked_key>::const_iterator& key_iter,
    const auto& precision) -> typename transform_component_t<TComponent>::optional_type
{
  const clip_uncooked_key& key{key_iter->second};

  if (clip_uncooked_key_has_component<TComponent>(key)) {
    const auto& key_component{clip_uncooked_key_get_component<TComponent>(key)};

    const auto prev_component_iter{find_key_component<TComponent>(
        std::make_reverse_iterator(key_iter), keys.rend(), [](const auto&) { return true; })};

    if (prev_component_iter == keys.rend()) {
      // This is a first key with this component
      return key_component;
    }

    const auto next_component_iter{find_key_component<TComponent>(
        std::next(key_iter), keys.cend(), [](const auto&) { return true; })};

    if (next_component_iter == keys.cend()) {
      // This is a last key with this component
      return key_component;
    }

    const auto& from{clip_uncooked_key_get_component<TComponent>(prev_component_iter->second)};
    const auto& to{clip_uncooked_key_get_component<TComponent>(next_component_iter->second)};

    const float time_range{next_component_iter->first - prev_component_iter->first};
    const float weight{(key_iter->first - prev_component_iter->first) / time_range};

    if constexpr (TComponent == transform_components::translation ||
                  TComponent == transform_components::scale) {
      const float3 lerped{float3_lerp(from, to, weight)};
      const bool reduced{float3_near(key_component, lerped, precision)};
      if (!reduced) {
        return key_component;
      }
    }
    else {
      static_assert(TComponent == transform_components::rotation);

      const quaternion lerped{quaternion_slerp(from, to, weight)};
      // TODO: use euler angle values for comparison
      const bool reduced{quaternion_near(key_component, lerped, precision)};
      if (!reduced) {
        return key_component;
      }
    }
  }

  return std::nullopt;
}

std::tuple<bool, bool, bool> compare_key_with_rest_pose(const clip_uncooked_key& key,
                                                        const transform& rest_pose_transform,
                                                        const track_reduction_precision& precision)
{
  bool translation_differs{false};
  bool rotation_differs{false};
  bool scale_differs{false};

  if (key.translation.has_value() &&
      !float3_near(key.translation.value(), rest_pose_transform.translation,
                   precision.translation)) {
    translation_differs = true;
  }

  if (key.rotation.has_value()) {
    // TODO: compare euler angles
    if (!quaternion_near(key.rotation.value(), rest_pose_transform.rotation,
                         precision.rotation_rad)) {
      rotation_differs = true;
    }
  }

  if (key.scale.has_value() &&
      !float3_near(key.scale.value(), rest_pose_transform.scale, precision.scale)) {
    scale_differs = true;
  }

  return {translation_differs, rotation_differs, scale_differs};
}

std::vector<clip_uncooked_track> remove_rest_pose_keys(
    const std::vector<clip_uncooked_track>& tracks,
    const skeleton& skeleton,
    const track_reduction_precision& precision)
{
  std::vector<clip_uncooked_track> reduced_tracks;

  for (const clip_uncooked_track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    const transform& rest_pose_transform{skeleton.get_rest_pose_transforms()[joint_index]};

    bool track_translation_differs{false};
    bool track_rotation_differs{false};
    bool track_scale_differs{false};

    for (const auto& [_, key] : track.keys) {
      const auto [key_translation_differs, key_rotation_differs, key_scale_differs] =
          compare_key_with_rest_pose(key, rest_pose_transform, precision);

      track_translation_differs |= key_translation_differs;
      track_rotation_differs |= key_rotation_differs;
      track_scale_differs |= key_scale_differs;
    }

    if (!track_translation_differs && !track_rotation_differs && !track_scale_differs) {
      // All track transforms match the rest pose, do not output it
      continue;
    }

    clip_uncooked_track reduced_track;
    reduced_track.joint_id = track.joint_id;

    for (const auto& [time, key] : track.keys) {
      if (track_translation_differs) {
        reduced_track.keys[time].translation = key.translation;
      }

      if (track_rotation_differs) {
        reduced_track.keys[time].rotation = key.rotation;
      }

      if (track_scale_differs) {
        reduced_track.keys[time].scale = key.scale;
      }
    }

    reduced_tracks.push_back(reduced_track);
  }

  return reduced_tracks;
}

std::vector<clip_uncooked_track> linear_key_reduction(
    const std::vector<clip_uncooked_track>& tracks,
    const track_reduction_precision& precision)
{
  std::vector<clip_uncooked_track> reduced_tracks;

  for (const clip_uncooked_track& track : tracks) {
    clip_uncooked_track reduced_track;
    reduced_track.joint_id = track.joint_id;

    for (auto key_iter{track.keys.cbegin()}; key_iter != track.keys.cend(); ++key_iter) {
      const float& time_s{key_iter->first};
      const clip_uncooked_key& key{key_iter->second};

      clip_uncooked_key reduced_key;

      // First and last component are always copied as-is
      // Components in between are compared to their interpolated counterpart,
      // and if they're close enough, omit them
      // TODO: current algorithm does not take hierarchy into account

      reduced_key.translation = linear_key_reduction_component<transform_components::translation>(
          track.keys, key_iter, precision.translation);

      reduced_key.rotation = linear_key_reduction_component<transform_components::rotation>(
          track.keys, key_iter, precision.rotation_rad);

      reduced_key.scale = linear_key_reduction_component<transform_components::scale>(
          track.keys, key_iter, precision.scale);

      // Add key to reduced track if it has data

      if (reduced_key.translation.has_value() || reduced_key.rotation.has_value() ||
          reduced_key.scale.has_value()) {
        reduced_track.keys.insert({time_s, reduced_key});
      }
    }

    if (!reduced_track.keys.empty()) {
      reduced_tracks.push_back(std::move(reduced_track));
    }
  }

  return reduced_tracks;
}

bool key_component_compare(const key_component& a, const key_component& b)
{
  // See comments in `cook_clip` why sorting is done this way.

  if (a.time_to_use < b.time_to_use) {
    return true;
  }

  if (a.time_to_use > b.time_to_use) {
    return false;
  }

  if (a.joint_index < b.joint_index) {
    return true;
  }

  if (a.joint_index > b.joint_index) {
    return false;
  }

  if (a.time < b.time) {
    return true;
  }

  if (a.time > b.time) {
    return false;
  }

  if (a.component < b.component) {
    return true;
  }

  if (a.component > b.component) {
    return false;
  }

  return false;
}

std::vector<key_component> key_component_collect(const std::vector<clip_uncooked_track>& tracks,
                                                 const skeleton& skeleton)
{
  EXPECTS(!tracks.empty());

  std::vector<key_component> keys;

  std::vector<key_component> translation_keys;
  std::vector<key_component> rotation_keys;
  std::vector<key_component> scale_keys;

  for (const clip_uncooked_track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    // Convert track to list of `key_component`s, per component type

    auto get_time_to_use_for_next_key = [](const std::vector<key_component>& v) {
      if (v.empty()) {
        return 0.0F;
      }

      return v.back().time;
    };

    for (const auto& [time, uncooked_key] : track.keys) {
      if (uncooked_key.translation.has_value()) {
        const float3& translation{uncooked_key.translation.value()};
        key_component key{.joint_index = joint_index,
                          .time = time,
                          .component = transform_components::translation,
                          .data = float4{translation.x, translation.y, translation.z},
                          .time_to_use = get_time_to_use_for_next_key(translation_keys)};
        translation_keys.push_back(key);
      }

      if (uncooked_key.rotation.has_value()) {
        const quaternion& rotation{uncooked_key.rotation.value()};
        key_component key{.joint_index = joint_index,
                          .time = time,
                          .component = transform_components::rotation,
                          .data = float4{rotation.x, rotation.y, rotation.z, rotation.w},
                          .time_to_use = get_time_to_use_for_next_key(rotation_keys)};
        rotation_keys.push_back(key);
      }

      if (uncooked_key.scale.has_value()) {
        const float3& scale{uncooked_key.scale.value()};
        key_component key{.joint_index = joint_index,
                          .time = time,
                          .component = transform_components::scale,
                          .data = float4{scale.x, scale.y, scale.z},
                          .time_to_use = get_time_to_use_for_next_key(scale_keys)};
        scale_keys.push_back(key);
      }
    }

    keys.insert(keys.end(), translation_keys.cbegin(), translation_keys.cend());
    keys.insert(keys.end(), rotation_keys.cbegin(), rotation_keys.cend());
    keys.insert(keys.end(), scale_keys.cbegin(), scale_keys.cend());

    translation_keys.clear();
    rotation_keys.clear();
    scale_keys.clear();
  }

  std::sort(keys.begin(), keys.end(), key_component_compare);

  return keys;
}
}  // namespace eely::internal