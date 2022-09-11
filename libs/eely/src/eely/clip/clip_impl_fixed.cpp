#include "eely/clip/clip_impl_fixed.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/clip/clip_cooking_none_fixed.h"
#include "eely/clip/clip_player_fixed.h"
#include "eely/clip/clip_utils.h"
#include "eely/math/quantization.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <vector>

namespace eely::internal {
static void joints_ranges_collect(const std::vector<clip_uncooked_track>& tracks,
                                  const skeleton& skeleton,
                                  std::vector<joint_range>& out_joints_ranges)
{
  for (const clip_uncooked_track& track : tracks) {
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
      out_joints_ranges.push_back(
          {.joint_index = joint_index,
           .range_translation_from = range_translation_from,
           .range_translation_length = range_translation_to - range_translation_from,
           .range_scale_from = range_scale_from,
           .range_scale_length = range_scale_to - range_scale_from});
    }
  }

  std::sort(out_joints_ranges.begin(), out_joints_ranges.end(),
            [](const auto& a, const auto& b) { return a.joint_index < b.joint_index; });
}

static void write_cooked_key(const cooked_key& key,
                             const clip_metadata_fixed& metadata,
                             std::vector<uint16_t>& out_data)
{
  uint16_t flags_and_joint_index{0};
  std::optional<uint16_t> time_s;
  std::optional<std::array<uint16_t, 3>> translation;
  std::optional<std::array<uint16_t, 4>> rotation;
  std::optional<std::array<uint16_t, 3>> scale;

  if (key.joint_index_changed) {
    flags_and_joint_index |= compression_key_flags::has_joint_index;

    static_assert(bits_joints_count <= 11);
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
      std::find_if(metadata.joints_ranges.begin(), metadata.joints_ranges.end(),
                   [&key](const auto& j) { return j.joint_index == key.joint_index; });

  const joint_range* joint_range{
      joint_metadata_iter == metadata.joints_ranges.end() ? nullptr : &(*joint_metadata_iter)};

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
    out_data.insert(out_data.end(), translation.value().begin(), translation.value().end());
  }

  if (rotation.has_value()) {
    out_data.insert(out_data.end(), rotation.value().begin(), rotation.value().end());
  }

  if (scale.has_value()) {
    out_data.insert(out_data.end(), scale.value().begin(), scale.value().end());
  }
}

clip_impl_fixed::clip_impl_fixed(bit_reader& reader)
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
    j.components = gsl::narrow<int>(reader.read(bits_transform_components));

    _metadata.joints_components[i] = j;
  }

  const gsl::index metadata_joint_ranges_size{reader.read(bits_joints_count)};
  _metadata.joints_ranges.resize(metadata_joint_ranges_size);
  for (gsl::index i{0}; i < metadata_joint_ranges_size; ++i) {
    joint_range j;

    j.joint_index = reader.read(bits_joints_count);
    j.range_translation_from = bit_cast<float>(reader.read(32));
    j.range_translation_length = bit_cast<float>(reader.read(32));
    j.range_scale_from = bit_cast<float>(reader.read(32));
    j.range_scale_length = bit_cast<float>(reader.read(32));

    _metadata.joints_ranges[i] = j;
  }

  // Data

  const gsl::index data_size{reader.read(32)};
  EXPECTS(data_size > 0);

  _data.resize(data_size);
  for (gsl::index i{0}; i < data_size; ++i) {
    _data[i] = reader.read(16);
  }
}

clip_impl_fixed::clip_impl_fixed(const float duration_s,
                                 const std::vector<clip_uncooked_track>& tracks,
                                 const bool is_additive,
                                 const skeleton& skeleton)
{
  std::vector<clip_uncooked_track> reduced_tracks{remove_rest_pose_keys(tracks, skeleton)};

  // TODO: linear key reduction

  // Metadata

  _metadata.duration_s = duration_s;
  _metadata.is_additive = is_additive;
  joint_components_collect(reduced_tracks, skeleton, _metadata.joints_components);
  joints_ranges_collect(tracks, skeleton, _metadata.joints_ranges);

  // Data

  const auto writer = [this](const cooked_key& key) { write_cooked_key(key, _metadata, _data); };
  clip_cook(reduced_tracks, skeleton, writer);
}

void clip_impl_fixed::serialize(bit_writer& writer) const
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

  writer.write({.value = gsl::narrow<uint32_t>(_metadata.joints_ranges.size()),
                .size_bits = bits_joints_count});

  for (const joint_range& j : _metadata.joints_ranges) {
    writer.write({.value = gsl::narrow<uint32_t>(j.joint_index), .size_bits = bits_joints_count});

    writer.write({.value = bit_cast<uint32_t>(j.range_translation_from), .size_bits = 32});
    writer.write({.value = bit_cast<uint32_t>(j.range_translation_length), .size_bits = 32});
    writer.write({.value = bit_cast<uint32_t>(j.range_scale_from), .size_bits = 32});
    writer.write({.value = bit_cast<uint32_t>(j.range_scale_length), .size_bits = 32});
  }

  // Data

  writer.write({.value = gsl::narrow<uint32_t>(_data.size()), .size_bits = 32});
  for (uint16_t d : _data) {
    writer.write({.value = d, .size_bits = 16});
  }
}

const clip_metadata_base* clip_impl_fixed::get_metadata() const
{
  return &_metadata;
}

std::unique_ptr<clip_player_base> clip_impl_fixed::create_player() const
{
  return std::make_unique<clip_player_fixed>(_metadata, _data);
}
}  // namespace eely::internal