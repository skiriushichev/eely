#include "eely/clip/clip_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/clip/clip_compression_scheme.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_uncooked.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <map>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
namespace internal {
static constexpr gsl::index bits_keys_count{16};
}

clip_uncooked::clip_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  using namespace eely::internal;

  _target_skeleton_id = string_id_deserialize(reader);

  _compression_scheme =
      static_cast<clip_compression_scheme>(reader.read(bits_clip_compression_scheme));

  const gsl::index tracks_count{reader.read(bits_joints_count)};
  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    clip_uncooked_track t;

    t.joint_id = string_id_deserialize(reader);

    const gsl::index keys_count{reader.read(bits_keys_count)};
    for (gsl::index key_index{0}; key_index < keys_count; ++key_index) {
      const float time_s{bit_cast<float>(reader.read(32))};

      clip_uncooked_key k;

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

    _tracks.push_back(std::move(t));
  }
}

clip_uncooked::clip_uncooked(const string_id& id)
    : resource_uncooked(id), _compression_scheme(clip_compression_scheme::fixed)
{
}

void clip_uncooked::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  string_id_serialize(_target_skeleton_id, writer);

  writer.write({.value = static_cast<uint32_t>(_compression_scheme),
                .size_bits = bits_clip_compression_scheme});

  const gsl::index tracks_count{std::ssize(_tracks)};
  EXPECTS(tracks_count <= joints_max_count);
  writer.write({.value = gsl::narrow<uint32_t>(tracks_count), .size_bits = bits_joints_count});

  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    const clip_uncooked_track& t{_tracks[track_index]};

    string_id_serialize(t.joint_id, writer);

    const gsl::index keys_count{std::ssize(t.keys)};
    writer.write({.value = gsl::narrow<uint32_t>(keys_count), .size_bits = bits_keys_count});

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

void clip_uncooked::collect_dependencies(std::unordered_set<string_id>& out_dependencies) const
{
  out_dependencies.insert(_target_skeleton_id);
}

const string_id& clip_uncooked::get_target_skeleton_id() const
{
  return _target_skeleton_id;
}

void clip_uncooked::set_target_skeleton_id(string_id skeleton_id)
{
  _target_skeleton_id = std::move(skeleton_id);
}

const std::vector<clip_uncooked_track>& clip_uncooked::get_tracks() const
{
  return _tracks;
}

void clip_uncooked::set_tracks(std::vector<clip_uncooked_track> tracks)
{
  _tracks = std::move(tracks);
}

clip_compression_scheme clip_uncooked::get_compression_scheme() const
{
  return _compression_scheme;
}

void clip_uncooked::set_compression_scheme(const clip_compression_scheme scheme)
{
  _compression_scheme = scheme;
}

float clip_uncooked::get_duration_s() const
{
  float duration_s{0.0F};

  for (const clip_uncooked_track& t : _tracks) {
    if (t.keys.empty()) {
      continue;
    }

    duration_s = std::max(duration_s, t.keys.rbegin()->first);
  }

  return duration_s;
}

clip_additive_uncooked::clip_additive_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  using namespace eely::internal;

  _target_skeleton_id = string_id_deserialize(reader);
  _skeleton_mask_id = string_id_deserialize(reader);

  _base_clip_id = string_id_deserialize(reader);
  _source_clip_id = string_id_deserialize(reader);

  const bool has_base_clip_range{static_cast<bool>(reader.read(1))};
  if (has_base_clip_range) {
    _base_clip_range =
        range{.from_s = bit_cast<float>(reader.read(32)), .to_s = bit_cast<float>(reader.read(32))};
  }

  const bool has_source_clip_range{static_cast<bool>(reader.read(1))};
  if (has_source_clip_range) {
    _source_clip_range =
        range{.from_s = bit_cast<float>(reader.read(32)), .to_s = bit_cast<float>(reader.read(32))};
  }

  _compression_scheme =
      static_cast<clip_compression_scheme>(reader.read(bits_clip_compression_scheme));
}

clip_additive_uncooked::clip_additive_uncooked(const string_id& id)
    : resource_uncooked(id), _compression_scheme(clip_compression_scheme::fixed)
{
}

void clip_additive_uncooked::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  string_id_serialize(_target_skeleton_id, writer);
  string_id_serialize(_skeleton_mask_id, writer);

  string_id_serialize(_base_clip_id, writer);
  string_id_serialize(_source_clip_id, writer);

  if (_base_clip_range.has_value()) {
    writer.write({.value = 1, .size_bits = 1});
    writer.write({.value = bit_cast<uint32_t>(_base_clip_range->from_s), .size_bits = 32});
    writer.write({.value = bit_cast<uint32_t>(_base_clip_range->to_s), .size_bits = 32});
  }
  else {
    writer.write({.value = 0, .size_bits = 1});
  }

  if (_source_clip_range.has_value()) {
    writer.write({.value = 1, .size_bits = 1});
    writer.write({.value = bit_cast<uint32_t>(_source_clip_range->from_s), .size_bits = 32});
    writer.write({.value = bit_cast<uint32_t>(_source_clip_range->to_s), .size_bits = 32});
  }
  else {
    writer.write({.value = 0, .size_bits = 1});
  }

  writer.write({.value = static_cast<uint32_t>(_compression_scheme),
                .size_bits = bits_clip_compression_scheme});
}

void clip_additive_uncooked::collect_dependencies(
    std::unordered_set<string_id>& out_dependencies) const
{
  out_dependencies.insert(_target_skeleton_id);
  out_dependencies.insert(_skeleton_mask_id);
  out_dependencies.insert(_base_clip_id);
  out_dependencies.insert(_source_clip_id);
}

const string_id& clip_additive_uncooked::get_target_skeleton_id() const
{
  return _target_skeleton_id;
}

void clip_additive_uncooked::set_target_skeleton_id(string_id skeleton_id)
{
  _target_skeleton_id = std::move(skeleton_id);
}

void clip_additive_uncooked::set_skeleton_mask_id(string_id skeleton_mask_id)
{
  _skeleton_mask_id = std::move(skeleton_mask_id);
}

string_id clip_additive_uncooked::get_skeleton_mask_id() const
{
  return _skeleton_mask_id;
}

void clip_additive_uncooked::set_base_clip_id(string_id base_clip_id)
{
  _base_clip_id = std::move(base_clip_id);
}

const string_id& clip_additive_uncooked::get_base_clip_id() const
{
  return _base_clip_id;
}

void clip_additive_uncooked::set_source_clip_id(string_id source_clip_id)
{
  _source_clip_id = std::move(source_clip_id);
}

const string_id& clip_additive_uncooked::get_source_clip_id() const
{
  return _source_clip_id;
}

void clip_additive_uncooked::set_base_clip_range(const std::optional<range>& base_clip_range)
{
  _base_clip_range = base_clip_range;
}

const std::optional<clip_additive_uncooked::range>& clip_additive_uncooked::get_base_clip_range()
    const
{
  return _base_clip_range;
}

void clip_additive_uncooked::set_source_clip_range(const std::optional<range>& source_clip_range)
{
  _source_clip_range = source_clip_range;
}

const std::optional<clip_additive_uncooked::range>& clip_additive_uncooked::get_source_clip_range()
    const
{
  return _source_clip_range;
}

clip_compression_scheme clip_additive_uncooked::get_compression_scheme() const
{
  return _compression_scheme;
}

void clip_additive_uncooked::set_compression_scheme(const clip_compression_scheme scheme)
{
  _compression_scheme = scheme;
}
}  // namespace eely