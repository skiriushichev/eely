#include "eely/clip_uncooked.h"

#include "eely/base_utils.h"
#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/float3.h"
#include "eely/quaternion.h"
#include "eely/resource_uncooked.h"
#include "eely/skeleton_uncooked.h"
#include "eely/string_id.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <map>
#include <optional>
#include <vector>

namespace eely {
static constexpr gsl::index bits_tracks_count{
    std::bit_width<size_t>(skeleton_uncooked::max_joints_count)};
static constexpr gsl::index bits_keys_count{16};

clip_uncooked::clip_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  _target_skeleton_id = string_id_deserialize(reader);

  const gsl::index tracks_count{reader.read(bits_tracks_count)};
  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    track t;

    t.joint_id = string_id_deserialize(reader);

    const gsl::index keys_count{reader.read(bits_keys_count)};
    for (gsl::index key_index{0}; key_index < keys_count; ++key_index) {
      const float time_s{bit_cast<float>(reader.read(32))};

      key k;

      const uint32_t has_translation{reader.read(1)};
      if (has_translation == 1) {
        k.translation = float3_deserialize(reader);
      }

      const uint32_t has_rotation{reader.read(1)};
      if (has_rotation == 1) {
        k.rotation = quaternion_deserialize(reader);
      }

      const uint32_t has_scale{reader.read(1)};
      if (has_scale == 1) {
        k.scale = float3_deserialize(reader);
      }

      t.keys[time_s] = k;
    }
  }
}

clip_uncooked::clip_uncooked(const string_id& id) : resource_uncooked(id) {}

void clip_uncooked::serialize(bit_writer& writer) const
{
  resource_uncooked::serialize(writer);

  string_id_serialize(_target_skeleton_id, writer);

  const gsl::index tracks_count{gsl::narrow<gsl::index>(_tracks.size())};
  Expects(tracks_count <= skeleton_uncooked::max_joints_count);
  writer.write({.value = gsl::narrow_cast<uint32_t>(tracks_count), .size_bits = bits_tracks_count});

  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    const track& t{_tracks[track_index]};

    string_id_serialize(t.joint_id, writer);

    const gsl::index keys_count{gsl::narrow<gsl::index>(t.keys.size())};
    Expects(std::bit_width(gsl::narrow<size_t>(t.keys.size())) <= bits_keys_count);
    writer.write({.value = gsl::narrow_cast<uint32_t>(keys_count), .size_bits = bits_keys_count});

    for (const auto& [time, key] : t.keys) {
      writer.write({.value = bit_cast<uint32_t>(time), .size_bits = 32});

      if (key.translation.has_value()) {
        writer.write({.value = 1, .size_bits = 1});
        float3_serialize(key.translation.value(), writer);
      }
      else {
        writer.write({.value = 0, .size_bits = 1});
      }

      if (key.rotation.has_value()) {
        writer.write({.value = 1, .size_bits = 1});
        quaternion_serialize(key.rotation.value(), writer);
      }
      else {
        writer.write({.value = 0, .size_bits = 1});
      }

      if (key.scale.has_value()) {
        writer.write({.value = 1, .size_bits = 1});
        float3_serialize(key.scale.value(), writer);
      }
      else {
        writer.write({.value = 0, .size_bits = 1});
      }
    }
  }
}

void clip_uncooked::collect_dependencies(std::vector<string_id>& out_dependencies) const
{
  out_dependencies.push_back(_target_skeleton_id);
}

const string_id& clip_uncooked::get_target_skeleton_id() const
{
  return _target_skeleton_id;
}

const std::vector<clip_uncooked::track>& clip_uncooked::get_tracks() const
{
  return _tracks;
}

float clip_uncooked::get_duration_s() const
{
  float duration_s{0.0F};

  for (const track& t : _tracks) {
    if (t.keys.empty()) {
      continue;
    }

    duration_s = std::max(duration_s, t.keys.rbegin()->first);
  }

  return duration_s;
}

void clip_uncooked::set_target_skeleton_id(string_id skeleton_id)
{
  _target_skeleton_id = std::move(skeleton_id);
}

void clip_uncooked::set_tracks(std::vector<track> tracks)
{
  _tracks = std::move(tracks);
}
}  // namespace eely