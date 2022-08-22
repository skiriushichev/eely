#include "eely/clip/clip_impl_none.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/clip/clip_cooking_none_fixed.h"
#include "eely/clip/clip_cursor.h"
#include "eely/clip/clip_player_none.h"
#include "eely/clip/clip_utils.h"

#include <gsl/narrow>

#include <memory>
#include <vector>

namespace eely::internal {
static void write_cooked_key(const cooked_key& key, std::vector<uint32_t>& data)
{
  uint32_t flags_and_joint_index{0};
  std::optional<uint32_t> time_s;
  std::optional<std::array<uint32_t, 3>> translation;
  std::optional<std::array<uint32_t, 4>> rotation;
  std::optional<std::array<uint32_t, 3>> scale;

  if (key.joint_index_changed) {
    flags_and_joint_index |= compression_key_flags::has_joint_index;

    const uint32_t joint_index{gsl::narrow_cast<uint32_t>(key.joint_index)};
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

  data.push_back(flags_and_joint_index);

  if (time_s.has_value()) {
    data.push_back(time_s.value());
  }

  if (translation.has_value()) {
    data.insert(data.end(), translation.value().begin(), translation.value().end());
  }

  if (rotation.has_value()) {
    data.insert(data.end(), rotation.value().begin(), rotation.value().end());
  }

  if (scale.has_value()) {
    data.insert(data.end(), scale.value().begin(), scale.value().end());
  }
}

clip_impl_none::clip_impl_none(bit_reader& reader)
{
  // Metadata

  _metadata.duration_s = bit_cast<float>(reader.read(32));
  EXPECTS(_metadata.duration_s >= 0.0F);

  _metadata.is_additive = reader.read(1) == 1U;

  const gsl::index metadata_joint_components_size{reader.read(bits_joints_count)};
  _metadata.joints_components.resize(metadata_joint_components_size);
  for (gsl::index i{0}; i < metadata_joint_components_size; ++i) {
    joint_components j;
    j.joint_index = reader.read(bits_joints_count);
    j.components = gsl::narrow_cast<int>(reader.read(bits_transform_components));

    _metadata.joints_components[i] = j;
  }

  // Data

  const gsl::index data_size{reader.read(32)};
  EXPECTS(data_size > 0);

  _data.resize(data_size);
  for (gsl::index i{0}; i < data_size; ++i) {
    _data[i] = reader.read(32);
  }
}

clip_impl_none::clip_impl_none(const float duration_s,
                               const std::vector<clip_uncooked_track>& tracks,
                               const bool is_additive,
                               const skeleton& skeleton)
{
  std::vector<clip_uncooked_track> reduced_tracks{remove_rest_pose_keys(tracks, skeleton)};
  reduced_tracks = linear_key_reduction(reduced_tracks);

  // Metadata

  _metadata.duration_s = duration_s;
  _metadata.is_additive = is_additive;
  joint_components_collect(reduced_tracks, skeleton, _metadata.joints_components);

  // Data

  const auto writer = [this](const cooked_key& key) { write_cooked_key(key, _data); };
  clip_cook(reduced_tracks, skeleton, writer);
}

void clip_impl_none::serialize(bit_writer& writer) const
{
  // Metadata

  writer.write({.value = bit_cast<uint32_t>(_metadata.duration_s), .size_bits = 32});
  writer.write({.value = _metadata.is_additive ? 1U : 0U, .size_bits = 1});

  writer.write({.value = gsl::narrow<uint32_t>(_metadata.joints_components.size()),
                .size_bits = bits_joints_count});
  for (const joint_components& j : _metadata.joints_components) {
    writer.write({.value = gsl::narrow<uint32_t>(j.joint_index), .size_bits = bits_joints_count});
    writer.write(
        {.value = gsl::narrow<uint32_t>(j.components), .size_bits = bits_transform_components});
  }

  // Data

  writer.write({.value = gsl::narrow<uint32_t>(_data.size()), .size_bits = 32});
  for (uint32_t d : _data) {
    writer.write({.value = d, .size_bits = 32});
  }
}

const clip_metadata_base* clip_impl_none::get_metadata() const
{
  return &_metadata;
}

std::unique_ptr<clip_player_base> clip_impl_none::create_player() const
{
  return std::make_unique<clip_player_none>(_metadata, _data);
}
}  // namespace eely::internal