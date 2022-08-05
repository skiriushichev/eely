#include "eely/clip.h"

#include "eely/assert.h"
#include "eely/base_utils.h"
#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/clip_player_compressed_fixed.h"
#include "eely/clip_player_uncompressed.h"
#include "eely/clip_uncooked.h"
#include "eely/clip_utils.h"
#include "eely/float4.h"
#include "eely/project.h"
#include "eely/quaternion.h"
#include "eely/resource.h"
#include "eely/skeleton.h"
#include "eely/skeleton_uncooked.h"
#include "eely/string_id.h"
#include "eely/transform.h"

#include <bit>
#include <cstdint>
#include <limits>
#include <memory>
#include <variant>
#include <vector>

namespace eely {
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

// Final key to be written in a buffer.
struct key_final final {
  // Joint index is not optional because for compressed data we need index
  // even though it didn't change from previous key, to get joint's metadata.
  gsl::index joint_index{0};
  bool joint_index_changed{false};

  std::optional<float> time_s;
  std::optional<float3> translation;
  std::optional<quaternion> rotation;
  std::optional<float3> scale;
};

static std::vector<clip_uncooked::track> reduce_tracks(
    const std::vector<clip_uncooked::track>& tracks,
    const skeleton& skeleton)
{
  std::vector<clip_uncooked::track> reduced_tracks;

  for (const clip_uncooked::track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    const transform& rest_pose_transform{skeleton.get_rest_pose_transforms()[joint_index]};

    bool translation_differs_from_rest_pose{false};
    bool rotation_differs_from_rest_pose{false};
    bool scale_differs_from_rest_pose{false};

    // TODO: setuppable?
    const float epsilon_rest_pose_comparison{1e-3F};

    for (const auto& [time, key] : track.keys) {
      if (key.translation.has_value() &&
          !float3_near(key.translation.value(), rest_pose_transform.translation,
                       epsilon_rest_pose_comparison)) {
        translation_differs_from_rest_pose = true;
      }

      if (key.rotation.has_value() &&
          !quaternion_near(key.rotation.value(), rest_pose_transform.rotation,
                           epsilon_rest_pose_comparison)) {
        rotation_differs_from_rest_pose = true;
      }

      if (key.scale.has_value() && !float3_near(key.scale.value(), rest_pose_transform.scale,
                                                epsilon_rest_pose_comparison)) {
        scale_differs_from_rest_pose = true;
      }
    }

    if (!translation_differs_from_rest_pose && !rotation_differs_from_rest_pose &&
        !scale_differs_from_rest_pose) {
      continue;
    }

    clip_uncooked::track reduced_track;
    reduced_track.joint_id = track.joint_id;

    for (const auto& [time, key] : track.keys) {
      if (translation_differs_from_rest_pose) {
        reduced_track.keys[time].translation = key.translation;
      }

      if (rotation_differs_from_rest_pose) {
        reduced_track.keys[time].rotation = key.rotation;
      }

      if (scale_differs_from_rest_pose) {
        reduced_track.keys[time].scale = key.scale;
      }
    }

    reduced_tracks.push_back(reduced_track);
  }

  return reduced_tracks;
}

static void key_final_write_uncompressed(const key_final& key, std::vector<uint32_t>& out_data)
{
  uint32_t flags_and_joint_index{0};
  std::optional<uint32_t> time_s;
  std::optional<std::array<uint32_t, 3>> translation;
  std::optional<std::array<uint32_t, 4>> rotation;
  std::optional<std::array<uint32_t, 3>> scale;

  if (key.joint_index_changed) {
    flags_and_joint_index |= compression_key_flags::has_joint_index;

    const uint32_t joint_index{gsl::narrow<uint32_t>(key.joint_index)};
    flags_and_joint_index |= (joint_index << 21);  // 21 for joint index to occupy last 11 bits
  }

  if (key.time_s.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_time;

    time_s = bit_cast<uint32_t>(key.time_s.value());
  }

  EXPECTS(key.translation.has_value() || key.rotation.has_value() || key.scale.has_value());

  if (key.translation.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_translation;

    const float3& t{key.translation.value()};
    translation = {bit_cast<uint32_t>(t.x), bit_cast<uint32_t>(t.y), bit_cast<uint32_t>(t.z)};
  }

  if (key.rotation.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_rotation;

    const quaternion& r{key.rotation.value()};
    rotation = {bit_cast<uint32_t>(r.x), bit_cast<uint32_t>(r.y), bit_cast<uint32_t>(r.z),
                bit_cast<uint32_t>(r.w)};
  }

  if (key.scale.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_scale;

    const float3& s{key.scale.value()};
    scale = {bit_cast<uint32_t>(s.x), bit_cast<uint32_t>(s.y), bit_cast<uint32_t>(s.z)};
  }

  // Push elements

  out_data.push_back(flags_and_joint_index);

  if (time_s.has_value()) {
    out_data.push_back(time_s.value());
  }

  if (translation.has_value()) {
    for (uint32_t v : translation.value()) {
      out_data.push_back(v);
    }
  }

  if (rotation.has_value()) {
    for (uint32_t v : rotation.value()) {
      out_data.push_back(v);
    }
  }

  if (scale.has_value()) {
    for (uint32_t v : scale.value()) {
      out_data.push_back(v);
    }
  }
}

static void key_final_write_compressed_fixed(const key_final& key,
                                             const clip_metadata_compressed_fixed& metadata,
                                             std::vector<uint16_t>& out_data)
{
  uint16_t flags_and_joint_index{0};
  std::optional<uint16_t> time_s;
  std::optional<std::array<uint16_t, 3>> translation;
  std::optional<std::array<uint16_t, 4>> rotation;
  std::optional<std::array<uint16_t, 3>> scale;

  if (key.joint_index_changed) {
    flags_and_joint_index |= compression_key_flags::has_joint_index;

    static_assert(skeleton_uncooked::bits_joints_count <= 11);
    flags_and_joint_index |= (key.joint_index << 5);
  }

  if (key.time_s.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_time;

    time_s = float_quantize({.value = key.time_s.value(),
                             .bits_count = 16,
                             .range_from = 0.0F,
                             .range_length = metadata.duration_s});
  }

  EXPECTS(key.translation.has_value() || key.rotation.has_value() || key.scale.has_value());

  auto joint_metadata_iter =
      std::find_if(metadata.joint_ranges.begin(), metadata.joint_ranges.end(),
                   [&key](const auto& j) { return j.joint_index == key.joint_index; });

  const clip_metadata_compressed_fixed::joint_range* joint_range{
      joint_metadata_iter == metadata.joint_ranges.end() ? nullptr : &(*joint_metadata_iter)};

  if (key.translation.has_value()) {
    EXPECTS(joint_range != nullptr);

    flags_and_joint_index |= compression_key_flags::has_translation;

    const float3& t{key.translation.value()};

    float_quantize_params params{.bits_count = 16,
                                 .range_from = joint_range->range_translation_from,
                                 .range_length = joint_range->range_translation_length};

    translation = std::array<uint16_t, 3>{};

    params.value = t.x;
    translation.value()[0] = float_quantize(params);

    params.value = t.y;
    translation.value()[1] = float_quantize(params);

    params.value = t.z;
    translation.value()[2] = float_quantize(params);
  }

  if (key.rotation.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_rotation;

    const quaternion& r{key.rotation.value()};
    rotation = quaternion_quantize(r);
  }

  if (key.scale.has_value()) {
    EXPECTS(joint_range != nullptr);

    flags_and_joint_index |= compression_key_flags::has_scale;

    const float3& s{key.scale.value()};

    float_quantize_params params{.bits_count = 16,
                                 .range_from = joint_range->range_scale_from,
                                 .range_length = joint_range->range_scale_length};

    scale = std::array<uint16_t, 3>{};

    params.value = s.x;
    scale.value()[0] = float_quantize(params);

    params.value = s.y;
    scale.value()[1] = float_quantize(params);

    params.value = s.z;
    scale.value()[2] = float_quantize(params);
  }

  // Push elements

  out_data.push_back(flags_and_joint_index);

  if (time_s.has_value()) {
    out_data.push_back(time_s.value());
  }

  if (translation.has_value()) {
    for (uint16_t v : translation.value()) {
      out_data.push_back(v);
    }
  }

  if (rotation.has_value()) {
    for (uint16_t v : rotation.value()) {
      out_data.push_back(v);
    }
  }

  if (scale.has_value()) {
    for (uint16_t v : scale.value()) {
      out_data.push_back(v);
    }
  }
}

static bool key_component_compare(const key_component& a, const key_component& b)
{
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

static std::vector<key_component> key_component_collect(
    const std::vector<clip_uncooked::track>& tracks,
    const skeleton& skeleton)
{
  std::vector<key_component> keys;

  EXPECTS(!tracks.empty());

  std::vector<key_component> translation_keys;
  std::vector<key_component> rotation_keys;
  std::vector<key_component> scale_keys;

  for (const clip_uncooked::track& track : tracks) {
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

static clip_metadata collect_metadata(const float duration_s,
                                      const std::vector<clip_uncooked::track>& tracks,
                                      const skeleton& skeleton,
                                      clip_metadata& out_metadata)
{
  out_metadata.duration_s = duration_s;

  out_metadata.joint_components.reserve(tracks.size());

  for (const clip_uncooked::track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    int components{0};
    for (const auto& [_, key] : track.keys) {
      if (key.translation.has_value()) {
        components |= transform_components::translation;
      }

      if (key.rotation.has_value()) {
        components |= transform_components::rotation;
      }

      if (key.scale.has_value()) {
        components |= transform_components::scale;
      }
    }

    out_metadata.joint_components.push_back({.joint_index = joint_index, components = components});
  }

  std::sort(out_metadata.joint_components.begin(), out_metadata.joint_components.end(),
            [](const auto& a, const auto& b) { return a.joint_index < b.joint_index; });

  return out_metadata;
}

static void collect_compressed_fixed_metadata(const float duration_s,
                                              const std::vector<clip_uncooked::track>& tracks,
                                              const skeleton& skeleton,
                                              clip_metadata_compressed_fixed& out_metadata)
{
  collect_metadata(duration_s, tracks, skeleton, out_metadata);

  for (const clip_uncooked::track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    float range_translation_from{std::numeric_limits<float>::max()};
    float range_translation_to{std::numeric_limits<float>::min()};
    float range_scale_from{std::numeric_limits<float>::max()};
    float range_scale_to{std::numeric_limits<float>::min()};

    bool write_metadata{false};

    for (const auto& [_, key] : track.keys) {
      if (key.translation.has_value()) {
        write_metadata = true;

        range_translation_from = std::min(
            {range_translation_from, key.translation->x, key.translation->y, key.translation->z});
        range_translation_to = std::max(
            {range_translation_to, key.translation->x, key.translation->y, key.translation->z});
      }

      if (key.scale.has_value()) {
        write_metadata = true;

        range_scale_from = std::min({range_scale_from, key.scale->x, key.scale->y, key.scale->z});
        range_scale_to = std::max({range_scale_to, key.scale->x, key.scale->y, key.scale->z});
      }
    }

    if (write_metadata) {
      out_metadata.joint_ranges.push_back(
          {.joint_index = joint_index,
           .range_translation_from = range_translation_from,
           .range_translation_length = range_translation_to - range_translation_from,
           .range_scale_from = range_scale_from,
           .range_scale_length = range_scale_to - range_scale_from});
    }
  }

  std::sort(out_metadata.joint_ranges.begin(), out_metadata.joint_ranges.end(),
            [](const auto& a, const auto& b) { return a.joint_index < b.joint_index; });
}

clip::clip(const project& project, bit_reader& reader)
    : resource(reader), _skeleton{*project.get_resource<skeleton>(string_id_deserialize(reader))}
{
  const auto compression_scheme =
      static_cast<eely::compression_scheme>(reader.read(bits_compression_scheme));

  switch (compression_scheme) {
    case eely::compression_scheme::uncompressed: {
      _clip_variant = uncompressed_clip{};
    } break;

    case eely::compression_scheme::compressed_fixed: {
      _clip_variant = compressed_fixed_clip{};
    } break;

    default: {
      EXPECTS(false);
    } break;
  }

  // Metadata

  clip_metadata* metadata = nullptr;
  if (compression_scheme == compression_scheme::uncompressed) {
    metadata = &std::get<uncompressed_clip>(_clip_variant).metadata;
  }
  else {
    metadata = &std::get<compressed_fixed_clip>(_clip_variant).metadata;
  }

  metadata->duration_s = bit_cast<float>(reader.read(32));
  EXPECTS(metadata->duration_s >= 0.0F);

  const gsl::index metadata_joint_components_size{
      reader.read(skeleton_uncooked::bits_joints_count)};
  metadata->joint_components.resize(metadata_joint_components_size);
  for (gsl::index i{0}; i < metadata_joint_components_size; ++i) {
    clip_metadata::joint_transform_components j;

    j.joint_index = reader.read(skeleton_uncooked::bits_joints_count);
    j.components = gsl::narrow<int>(reader.read(bits_transform_components));

    metadata->joint_components[i] = j;
  }

  if (compression_scheme == compression_scheme::compressed_fixed) {
    clip_metadata_compressed_fixed* metadata_compressed_fixed{
        polymorphic_downcast<clip_metadata_compressed_fixed*>(metadata)};

    const gsl::index metadata_joint_ranges_size{reader.read(skeleton_uncooked::bits_joints_count)};
    metadata_compressed_fixed->joint_ranges.resize(metadata_joint_ranges_size);
    for (gsl::index i{0}; i < metadata_joint_ranges_size; ++i) {
      clip_metadata_compressed_fixed::joint_range j;

      j.joint_index = reader.read(skeleton_uncooked::bits_joints_count);
      j.range_translation_from = bit_cast<float>(reader.read(32));
      j.range_translation_length = bit_cast<float>(reader.read(32));
      j.range_scale_from = bit_cast<float>(reader.read(32));
      j.range_scale_length = bit_cast<float>(reader.read(32));

      metadata_compressed_fixed->joint_ranges[i] = j;
    }
  }

  // Data

  const size_t data_size{reader.read(32)};
  EXPECTS(data_size > 0);

  if (compression_scheme == compression_scheme::uncompressed) {
    std::vector<uint32_t>& data{std::get<uncompressed_clip>(_clip_variant).data};
    data.resize(data_size);
    for (gsl::index i{0}; i < data.size(); ++i) {
      data[i] = reader.read(32);
    }
  }
  else {
    std::vector<uint16_t>& data{std::get<compressed_fixed_clip>(_clip_variant).data};
    data.resize(data_size);
    for (gsl::index i{0}; i < data.size(); ++i) {
      data[i] = reader.read(16);
    }
  }
}

clip::clip(const project& project, const clip_uncooked& uncooked)
    : resource(uncooked.get_id()),
      _skeleton{*project.get_resource<skeleton>(uncooked.get_target_skeleton_id())}
{
  const compression_scheme compression_scheme{uncooked.get_compression_scheme()};

  const std::vector<clip_uncooked::track> tracks{reduce_tracks(uncooked.get_tracks(), _skeleton)};

  std::vector<key_component> key_components{key_component_collect(tracks, _skeleton)};

  // Init data variant

  switch (compression_scheme) {
    case eely::compression_scheme::uncompressed: {
      uncompressed_clip data;
      collect_metadata(uncooked.get_duration_s(), tracks, _skeleton, data.metadata);
      _clip_variant = data;
    } break;

    case eely::compression_scheme::compressed_fixed: {
      compressed_fixed_clip data;
      collect_compressed_fixed_metadata(uncooked.get_duration_s(), tracks, _skeleton,
                                        data.metadata);
      _clip_variant = data;
    } break;

    default: {
      EXPECTS(false);
    } break;
  }

  // Write keys in a single buffer
  // Combine multiple `key_ext` in a single key if they have the same joint index and times
  // Only write time and joint indices if they differ from a previous key

  auto key_final_write = [this, compression_scheme](const key_final& key) {
    if (compression_scheme == compression_scheme::uncompressed) {
      uncompressed_clip& storage{std::get<uncompressed_clip>(_clip_variant)};
      key_final_write_uncompressed(key, storage.data);
    }
    else {
      compressed_fixed_clip& storage{std::get<compressed_fixed_clip>(_clip_variant)};
      key_final_write_compressed_fixed(key, storage.metadata, storage.data);
    }
  };

  std::optional<float> previous_time;
  std::optional<float> previous_time_to_use;
  std::optional<gsl::index> previous_joint_index;

  key_final key_to_write;

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
    bool flush{time_changed || time_to_use_changed || joint_changed};

    if (flush) {
      key_final_write(key_to_write);
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
      key_final_write(key_to_write);
    }
  }
}

void clip::serialize(bit_writer& writer) const
{
  resource::serialize(writer);

  string_id_serialize(_skeleton.get_id(), writer);

  compression_scheme compression_scheme;
  if (std::holds_alternative<uncompressed_clip>(_clip_variant)) {
    compression_scheme = compression_scheme::uncompressed;
  }
  else {
    compression_scheme = compression_scheme::compressed_fixed;
  }

  writer.write(
      {.value = static_cast<uint32_t>(compression_scheme), .size_bits = bits_compression_scheme});

  // Metadata

  const clip_metadata* metadata = nullptr;
  if (compression_scheme == compression_scheme::uncompressed) {
    metadata = &std::get<uncompressed_clip>(_clip_variant).metadata;
  }
  else {
    metadata = &std::get<compressed_fixed_clip>(_clip_variant).metadata;
  }

  writer.write({.value = bit_cast<uint32_t>(metadata->duration_s), .size_bits = 32});

  writer.write({.value = gsl::narrow<uint32_t>(metadata->joint_components.size()),
                .size_bits = skeleton_uncooked::bits_joints_count});
  for (const clip_metadata::joint_transform_components& j : metadata->joint_components) {
    writer.write({.value = gsl::narrow<uint32_t>(j.joint_index),
                  .size_bits = skeleton_uncooked::bits_joints_count});
    writer.write(
        {.value = gsl::narrow<uint32_t>(j.components), .size_bits = bits_transform_components});
  }

  if (compression_scheme == compression_scheme::compressed_fixed) {
    const clip_metadata_compressed_fixed* metadata_compressed_fixed{
        polymorphic_downcast<const clip_metadata_compressed_fixed*>(metadata)};

    writer.write({.value = gsl::narrow<uint32_t>(metadata_compressed_fixed->joint_ranges.size()),
                  .size_bits = skeleton_uncooked::bits_joints_count});

    for (const clip_metadata_compressed_fixed::joint_range& j :
         metadata_compressed_fixed->joint_ranges) {
      writer.write({.value = gsl::narrow<uint32_t>(j.joint_index),
                    .size_bits = skeleton_uncooked::bits_joints_count});

      writer.write({.value = bit_cast<uint32_t>(j.range_translation_from), .size_bits = 32});
      writer.write({.value = bit_cast<uint32_t>(j.range_translation_length), .size_bits = 32});
      writer.write({.value = bit_cast<uint32_t>(j.range_scale_from), .size_bits = 32});
      writer.write({.value = bit_cast<uint32_t>(j.range_scale_length), .size_bits = 32});
    }
  }

  // Data

  if (compression_scheme == compression_scheme::uncompressed) {
    const std::vector<uint32_t>& data{std::get<uncompressed_clip>(_clip_variant).data};

    writer.write({.value = gsl::narrow<uint32_t>(data.size()), .size_bits = 32});
    for (uint32_t d : data) {
      writer.write({.value = d, .size_bits = 32});
    }
  }
  else {
    const std::vector<uint16_t>& data{std::get<compressed_fixed_clip>(_clip_variant).data};

    writer.write({.value = gsl::narrow<uint32_t>(data.size()), .size_bits = 32});
    for (uint16_t d : data) {
      writer.write({.value = d, .size_bits = 16});
    }
  }
}

float clip::get_duration_s() const
{
  if (std::holds_alternative<uncompressed_clip>(_clip_variant)) {
    const uncompressed_clip& storage{std::get<uncompressed_clip>(_clip_variant)};
    return storage.metadata.duration_s;
  }

  const compressed_fixed_clip& storage{std::get<compressed_fixed_clip>(_clip_variant)};
  return storage.metadata.duration_s;
}

std::variant<clip_metadata, clip_metadata_compressed_fixed> clip::get_metadata() const
{
  if (std::holds_alternative<uncompressed_clip>(_clip_variant)) {
    const uncompressed_clip& storage{std::get<uncompressed_clip>(_clip_variant)};
    return storage.metadata;
  }

  const compressed_fixed_clip& storage{std::get<compressed_fixed_clip>(_clip_variant)};
  return storage.metadata;
}

std::unique_ptr<clip_player_base> clip::create_player() const
{
  if (std::holds_alternative<uncompressed_clip>(_clip_variant)) {
    const uncompressed_clip& storage{std::get<uncompressed_clip>(_clip_variant)};
    return std::make_unique<clip_player_uncompressed>(storage.data, storage.metadata);
  }

  const compressed_fixed_clip& storage{std::get<compressed_fixed_clip>(_clip_variant)};
  return std::make_unique<clip_player_compressed_fixed>(storage.data, storage.metadata);
}
}  // namespace eely