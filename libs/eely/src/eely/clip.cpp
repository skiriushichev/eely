#include "eely/clip.h"

#include "eely/base_utils.h"
#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/clip_compression.h"
#include "eely/clip_player_uncompressed.h"
#include "eely/clip_uncooked.h"
#include "eely/float4.h"
#include "eely/project.h"
#include "eely/resource.h"
#include "eely/skeleton.h"
#include "eely/string_id.h"
#include "eely/transform.h"

#include <gsl/assert>

#include <cstdint>
#include <limits>
#include <memory>
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
  // If there is a third key at t=7.0, we need at 6th second,
  // since we need it for any t in [6.0, 7.0] interval.
  // So "time to use" = 0 if this is a first key, otherwise it is equal to previous key's time.
  float time_to_use{-1.0F};
};

// Final key to be written in a buffer
struct key_final final {
  std::optional<gsl::index> joint_index;
  std::optional<float> time_s;
  std::optional<float3> translation;
  std::optional<quaternion> rotation;
  std::optional<float3> scale;
};

void key_final_write(const key_final& key, std::vector<uint32_t>& out_data)
{
  uint32_t flags_and_joint_index{0};
  std::optional<uint32_t> time_s;
  std::optional<std::array<uint32_t, 3>> translation;
  std::optional<std::array<uint32_t, 4>> rotation;
  std::optional<std::array<uint32_t, 3>> scale;

  if (key.joint_index.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_joint_index;

    uint32_t joint_index{gsl::narrow<uint32_t>(key.joint_index.value())};
    flags_and_joint_index |= (joint_index << 16);
  }

  if (key.time_s.has_value()) {
    flags_and_joint_index |= compression_key_flags::has_time;

    time_s = bit_cast<uint32_t>(key.time_s.value());
  }

  Expects(key.translation.has_value() || key.rotation.has_value() || key.scale.has_value());

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

bool key_component_compare(const key_component& a, const key_component& b)
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

std::vector<key_component> key_component_collect(const clip_uncooked& uncooked,
                                                 const skeleton& skeleton)
{
  std::vector<key_component> keys_ext;

  const std::vector<clip_uncooked::track>& tracks{uncooked.get_tracks()};
  Expects(!tracks.empty());

  std::vector<key_component> translation_keys_ext;
  std::vector<key_component> rotation_keys_ext;
  std::vector<key_component> scale_keys_ext;

  for (const clip_uncooked::track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    Expects(joint_index_opt.has_value());

    const gsl::index joint_index{joint_index_opt.value()};

    // Convert track to extended keys per component type

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
                          .time_to_use = get_time_to_use_for_next_key(translation_keys_ext)};
        translation_keys_ext.push_back(key);
      }

      if (uncooked_key.rotation.has_value()) {
        const quaternion& rotation{uncooked_key.rotation.value()};
        key_component key{.joint_index = joint_index,
                          .time = time,
                          .component = transform_components::rotation,
                          .data = float4{rotation.x, rotation.y, rotation.z, rotation.w},
                          .time_to_use = get_time_to_use_for_next_key(rotation_keys_ext)};
        rotation_keys_ext.push_back(key);
      }

      if (uncooked_key.scale.has_value()) {
        const float3& scale{uncooked_key.scale.value()};
        key_component key{.joint_index = joint_index,
                          .time = time,
                          .component = transform_components::scale,
                          .data = float4{scale.x, scale.y, scale.z},
                          .time_to_use = get_time_to_use_for_next_key(scale_keys_ext)};
        scale_keys_ext.push_back(key);
      }
    }

    keys_ext.insert(keys_ext.end(), translation_keys_ext.cbegin(), translation_keys_ext.cend());
    keys_ext.insert(keys_ext.end(), rotation_keys_ext.cbegin(), rotation_keys_ext.cend());
    keys_ext.insert(keys_ext.end(), scale_keys_ext.cbegin(), scale_keys_ext.cend());

    translation_keys_ext.clear();
    rotation_keys_ext.clear();
    scale_keys_ext.clear();
  }

  std::sort(keys_ext.begin(), keys_ext.end(), key_component_compare);

  return keys_ext;
}

clip::clip(const project& project, bit_reader& reader)
    : resource(reader), _skeleton{*project.get_resource<skeleton>(string_id_deserialize(reader))}
{
  _duration_s = bit_cast<float>(reader.read(32));
  Expects(_duration_s >= 0.0F);

  _data.resize(reader.read(32));
  Expects(!_data.empty());

  for (gsl::index i{0}; i < _data.size(); ++i) {
    _data[i] = reader.read(32);
  }
}

clip::clip(const project& project, const clip_uncooked& uncooked)
    : resource(uncooked.get_id()),
      _skeleton{*project.get_resource<skeleton>(uncooked.get_target_skeleton_id())},
      _duration_s{uncooked.get_duration_s()}
{
  // Collect extended keys
  std::vector<key_component> key_components{key_component_collect(uncooked, _skeleton)};

  // Write keys in a single buffer
  // Combine multiple `key_ext` in a single key if they have the same joint index and times
  // Only write time and joint indices if they differ from a previous key

  std::optional<float> previous_time;
  std::optional<float> previous_time_to_use;
  std::optional<gsl::index> previous_joint_index;

  key_final key_to_write;

  Expects(!key_components.empty());
  for (gsl::index i{0}; i < key_components.size(); ++i) {
    const key_component& key_ext{key_components[i]};

    // Write data whenever we moved to a new time or a new index

    bool flush{false};

    if (previous_time.has_value() && key_ext.time != previous_time.value()) {
      flush = true;
    }

    if (previous_time_to_use.has_value() && key_ext.time_to_use != previous_time_to_use.value()) {
      flush = true;
    }

    if (previous_joint_index.has_value() && key_ext.joint_index != previous_joint_index.value()) {
      flush = true;
    }

    if (flush) {
      key_final_write(key_to_write, _data);
      key_to_write = {};
    }

    if (!previous_time.has_value() || key_ext.time != previous_time.value()) {
      key_to_write.time_s = key_ext.time;
    }

    if (!previous_joint_index.has_value() || key_ext.joint_index != previous_joint_index.value()) {
      key_to_write.joint_index = key_ext.joint_index;
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
    if (i == key_components.size() - 1) {
      key_final_write(key_to_write, _data);
    }
  }

  _data.shrink_to_fit();
}

void clip::serialize(bit_writer& writer) const
{
  resource::serialize(writer);

  string_id_serialize(_skeleton.get_id(), writer);

  writer.write({.value = bit_cast<uint32_t>(_duration_s), .size_bits = 32});

  writer.write({.value = gsl::narrow<uint32_t>(_data.size()), .size_bits = 32});
  for (uint32_t d : _data) {
    writer.write({.value = d, .size_bits = 32});
  }
}

std::unique_ptr<clip_player_base> clip::create_player() const
{
  return std::make_unique<clip_player_uncompressed>(std::span{_data}, _duration_s, _skeleton);
}
}  // namespace eely