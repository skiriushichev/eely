#include "eely/clip/clip_player_fixed.h"

#include "eely/base/base_utils.h"
#include "eely/clip/clip_cooking_none_fixed.h"
#include "eely/clip/clip_cursor.h"
#include "eely/clip/clip_impl_fixed.h"
#include "eely/clip/clip_utils.h"
#include "eely/math/quantization.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/narrow>
#include <gsl/util>

#include <cstdint>
#include <span>

namespace eely::internal {
clip_player_fixed::clip_player_fixed(const clip_metadata_fixed& metadata,
                                     std::span<const uint16_t> data)
    : _metadata(metadata), _data{data}
{
  cursor_init(_cursor, _metadata.joints_components);
}

float clip_player_fixed::get_duration_s()
{
  return _metadata.duration_s;
}

void clip_player_fixed::play(const float time_s, skeleton_pose& out_pose)
{
  using flags = compression_key_flags;

  if (_metadata.is_additive) {
    out_pose.reset(skeleton_pose::type::additive);
  }
  else {
    out_pose.reset(skeleton_pose::type::absolute);
  }

  if (time_s < _cursor.last_play_time_s) {
    // Cursor cannot move backwards, need to start over
    cursor_reset(_cursor);
  }

  _cursor.last_play_time_s = time_s;

  gsl::index data_pos{_cursor.last_data_pos};

  gsl::index cursor_translation_index{0};
  gsl::index cursor_rotation_index{0};
  gsl::index cursor_scale_index{0};
  gsl::index metadata_index{0};

  const gsl::index data_size{std::ssize(_data)};

  while (data_pos < data_size) {
    const uint16_t header{_data[data_pos]};

    const bool has_joint_index{has_flag(header, flags::has_joint_index)};
    const bool has_time{has_flag(header, flags::has_time)};
    const bool has_translation{has_flag(header, flags::has_translation)};
    const bool has_rotation{has_flag(header, flags::has_rotation)};
    const bool has_scale{has_flag(header, flags::has_scale)};

    EXPECTS(has_translation || has_rotation || has_scale);

    if (has_joint_index) {
      _cursor.last_data_joint_index = header >> 5;
    }

    cursor_component<float3>* translation{nullptr};
    cursor_component<quaternion>* rotation{nullptr};
    cursor_component<float3>* scale{nullptr};

    bool outdated{false};

    if (has_translation) {
      translation = &get_by_joint_index(_cursor.translations, cursor_translation_index,
                                        _cursor.last_data_joint_index);
      outdated |= cursor_component_is_outdated(*translation, time_s);
    }

    if (has_rotation) {
      rotation = &get_by_joint_index(_cursor.rotations, cursor_rotation_index,
                                     _cursor.last_data_joint_index);
      outdated |= cursor_component_is_outdated(*rotation, time_s);
    }

    if (has_scale) {
      scale =
          &get_by_joint_index(_cursor.scales, cursor_scale_index, _cursor.last_data_joint_index);
      outdated |= cursor_component_is_outdated(*scale, time_s);
    }

    if (!outdated) {
      // If this joint is not outdated, others are not as well
      // This is guaranteed by the sorting
      break;
    }

    ++data_pos;

    if (has_time) {
      _cursor.last_data_time_s = float_dequantize({.data = _data[data_pos],
                                                   .bits_count = 16,
                                                   .range_from = 0.0F,
                                                   .range_length = _metadata.duration_s});
      ++data_pos;
    }

    if (has_translation) {
      const joint_range& joint_metadata{get_by_joint_index(_metadata.joints_ranges, metadata_index,
                                                           _cursor.last_data_joint_index)};

      float_dequantize_params params{.bits_count = 16,
                                     .range_from = joint_metadata.range_translation_from,
                                     .range_length = joint_metadata.range_translation_length};

      params.data = _data[data_pos + 0];
      const float x{float_dequantize(params)};

      params.data = _data[data_pos + 1];
      const float y{float_dequantize(params)};

      params.data = _data[data_pos + 2];
      const float z{float_dequantize(params)};

      data_pos += 3;

      cursor_component_advance(*translation, float3{x, y, z}, _cursor.last_data_time_s);
    }

    if (has_rotation) {
      quaternion value{quaternion_dequantize(_data.subspan(data_pos, 4))};
      data_pos += 4;

      cursor_component_advance(*rotation, value, _cursor.last_data_time_s);
    }

    if (has_scale) {
      const joint_range& joint_metadata{get_by_joint_index(_metadata.joints_ranges, metadata_index,
                                                           _cursor.last_data_joint_index)};

      float_dequantize_params params{.bits_count = 16,
                                     .range_from = joint_metadata.range_scale_from,
                                     .range_length = joint_metadata.range_scale_length};

      params.data = _data[data_pos + 0];
      const float x{float_dequantize(params)};

      params.data = _data[data_pos + 1];
      const float y{float_dequantize(params)};

      params.data = _data[data_pos + 2];
      const float z{float_dequantize(params)};

      data_pos += 3;

      cursor_component_advance(*scale, float3{x, y, z}, _cursor.last_data_time_s);
    }
  }

  _cursor.last_data_pos = data_pos;

  cursor_calculate_pose(_cursor, time_s, out_pose);
}
}  // namespace eely::internal