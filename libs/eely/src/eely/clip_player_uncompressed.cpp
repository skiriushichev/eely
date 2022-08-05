#include "eely/clip_player_uncompressed.h"

#include "eely/assert.h"
#include "eely/base_utils.h"
#include "eely/clip.h"
#include "eely/clip_player_base.h"
#include "eely/clip_utils.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"
#include "eely/transform.h"

#include <gsl/util>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <span>

namespace eely {
clip_player_uncompressed::clip_player_uncompressed(std::span<const uint32_t> data,
                                                   const clip_metadata& metadata)
    : _data{data}, _metadata{metadata}
{
  EXPECTS(!_metadata.joint_components.empty());

  _cursor.shallow_joint_index = _metadata.joint_components.front().joint_index;

  for (const auto& [index, components] : _metadata.joint_components) {
    EXPECTS(components != 0);

    if (has_flag(components, transform_components::translation)) {
      cursor_component<float3> component;
      component.joint_index = index;
      _cursor.translations.push_back(component);
    }

    if (has_flag(components, transform_components::rotation)) {
      cursor_component<quaternion> component;
      component.joint_index = index;
      _cursor.rotations.push_back(component);
    }

    if (has_flag(components, transform_components::scale)) {
      cursor_component<float3> component;
      component.joint_index = index;
      _cursor.scales.push_back(component);
    }
  }
}

float clip_player_uncompressed::get_duration_s()
{
  return _metadata.duration_s;
}

void clip_player_uncompressed::play(const float time_s, skeleton_pose& out_pose)
{
  using flags = compression_key_flags;

  if (time_s < _cursor.last_play_time_s) {
    // Cursor cannot move backwards, need to start over
    cursor_reset(_cursor);
  }

  _cursor.last_play_time_s = time_s;

  gsl::index data_pos{_cursor.last_data_pos};

  gsl::index cursor_translation_index{0};
  gsl::index cursor_rotation_index{0};
  gsl::index cursor_scale_index{0};

  while (data_pos < _data.size()) {
    const uint32_t header{_data[data_pos]};

    const bool has_joint_index{has_flag(header, flags::has_joint_index)};
    const bool has_time{has_flag(header, flags::has_time)};
    const bool has_translation{has_flag(header, flags::has_translation)};
    const bool has_rotation{has_flag(header, flags::has_rotation)};
    const bool has_scale{has_flag(header, flags::has_scale)};

    EXPECTS(has_translation || has_rotation || has_scale);

    if (has_joint_index) {
      _cursor.last_data_joint_index = header >> 21;
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
      _cursor.last_data_time_s = bit_cast<float>(_data[data_pos]);
      ++data_pos;
    }

    if (has_translation) {
      float3 value{bit_cast<float>(_data[data_pos + 0]), bit_cast<float>(_data[data_pos + 1]),
                   bit_cast<float>(_data[data_pos + 2])};
      data_pos += 3;

      cursor_component_advance(*translation, value, _cursor.last_data_time_s);
    }

    if (has_rotation) {
      quaternion value{bit_cast<float>(_data[data_pos + 0]), bit_cast<float>(_data[data_pos + 1]),
                       bit_cast<float>(_data[data_pos + 2]), bit_cast<float>(_data[data_pos + 3])};
      data_pos += 4;

      cursor_component_advance(*rotation, value, _cursor.last_data_time_s);
    }

    if (has_scale) {
      float3 value{bit_cast<float>(_data[data_pos + 0]), bit_cast<float>(_data[data_pos + 1]),
                   bit_cast<float>(_data[data_pos + 2])};
      data_pos += 3;

      cursor_component_advance(*scale, value, _cursor.last_data_time_s);
    }
  }

  _cursor.last_data_pos = data_pos;

  cursor_calculate_pose(_cursor, time_s, out_pose);
}
}  // namespace eely