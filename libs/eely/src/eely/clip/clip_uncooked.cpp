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
#include "eely/skeleton/skeleton_utils.h"

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

clip_uncooked::clip_uncooked(internal::bit_reader& reader) : resource_uncooked(reader)
{
  using namespace eely::internal;

  _target_skeleton_id = bit_reader_read<string_id>(reader);

  _skeleton_mask_id = bit_reader_read<string_id>(reader);

  _compression_scheme =
      bit_reader_read<clip_compression_scheme>(reader, bits_clip_compression_scheme);

  const auto tracks_count{bit_reader_read<gsl::index>(reader, bits_joints_count)};
  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    clip_uncooked_track t;

    t.joint_id = bit_reader_read<string_id>(reader);

    const auto keys_count{bit_reader_read<gsl::index>(reader, bits_keys_count)};
    for (gsl::index key_index{0}; key_index < keys_count; ++key_index) {
      const auto time_s{bit_reader_read<float>(reader)};

      clip_uncooked_key k;

      k.translation = bit_reader_read<std::optional<float3>>(reader);
      k.rotation = bit_reader_read<std::optional<quaternion>>(reader);
      k.scale = bit_reader_read<std::optional<float3>>(reader);

      t.keys[time_s] = k;
    }

    _tracks.push_back(std::move(t));
  }
}

clip_uncooked::clip_uncooked(const string_id& id)
    : resource_uncooked(id), _compression_scheme(clip_compression_scheme::fixed)
{
}

void clip_uncooked::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  bit_writer_write(writer, _target_skeleton_id);

  bit_writer_write(writer, _skeleton_mask_id);

  bit_writer_write(writer, _compression_scheme, bits_clip_compression_scheme);

  const gsl::index tracks_count{std::ssize(_tracks)};
  EXPECTS(tracks_count <= joints_max_count);
  bit_writer_write(writer, tracks_count, bits_joints_count);

  for (gsl::index track_index{0}; track_index < tracks_count; ++track_index) {
    const clip_uncooked_track& t{_tracks[track_index]};

    bit_writer_write(writer, t.joint_id);

    const gsl::index keys_count{std::ssize(t.keys)};
    bit_writer_write(writer, keys_count, bits_keys_count);

    for (const auto& [time, key] : t.keys) {
      bit_writer_write(writer, time);

      bit_writer_write(writer, key.translation);
      bit_writer_write(writer, key.rotation);
      bit_writer_write(writer, key.scale);
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

void clip_uncooked::set_skeleton_mask_id(string_id skeleton_mask_id)
{
  _skeleton_mask_id = std::move(skeleton_mask_id);
}

string_id clip_uncooked::get_skeleton_mask_id() const
{
  return _skeleton_mask_id;
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

clip_additive_uncooked::clip_additive_uncooked(internal::bit_reader& reader)
    : resource_uncooked(reader)
{
  using namespace eely::internal;

  _target_skeleton_id = bit_reader_read<string_id>(reader);
  _skeleton_mask_id = bit_reader_read<string_id>(reader);

  _base_clip_id = bit_reader_read<string_id>(reader);
  _source_clip_id = bit_reader_read<string_id>(reader);

  const auto has_base_clip_range{bit_reader_read<bool>(reader)};
  if (has_base_clip_range) {
    _base_clip_range =
        range{.from_s = bit_reader_read<float>(reader), .to_s = bit_reader_read<float>(reader)};
  }

  const auto has_source_clip_range{bit_reader_read<bool>(reader)};
  if (has_source_clip_range) {
    _source_clip_range =
        range{.from_s = bit_reader_read<float>(reader), .to_s = bit_reader_read<float>(reader)};
  }

  _compression_scheme =
      bit_reader_read<clip_compression_scheme>(reader, bits_clip_compression_scheme);
}

clip_additive_uncooked::clip_additive_uncooked(const string_id& id)
    : resource_uncooked(id), _compression_scheme(clip_compression_scheme::fixed)
{
}

void clip_additive_uncooked::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  bit_writer_write(writer, _target_skeleton_id);
  bit_writer_write(writer, _skeleton_mask_id);

  bit_writer_write(writer, _base_clip_id);
  bit_writer_write(writer, _source_clip_id);

  if (_base_clip_range.has_value()) {
    bit_writer_write(writer, true);
    bit_writer_write(writer, _base_clip_range->from_s);
    bit_writer_write(writer, _base_clip_range->to_s);
  }
  else {
    bit_writer_write(writer, false);
  }

  if (_source_clip_range.has_value()) {
    bit_writer_write(writer, true);
    bit_writer_write(writer, _source_clip_range->from_s);
    bit_writer_write(writer, _source_clip_range->to_s);
  }
  else {
    bit_writer_write(writer, false);
  }

  bit_writer_write(writer, _compression_scheme, bits_clip_compression_scheme);
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