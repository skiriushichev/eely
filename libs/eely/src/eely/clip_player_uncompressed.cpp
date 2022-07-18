#include "eely/clip_player_uncompressed.h"

#include "eely/base_utils.h"
#include "eely/clip.h"
#include "eely/clip_compression.h"
#include "eely/clip_player_base.h"
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
// Get component for specified joint.
// It is guaranteed, that there is such a component
// for every joint index present in animation data.
template <typename T>
cursor_component<T>& get_cursor_component(std::vector<cursor_component<T>>& components,
                                          gsl::index& components_index,
                                          const gsl::index joint_index)
{
  while (true) {
    cursor_component<T>& component{components[components_index]};
    if (components[components_index].joint_index == joint_index) {
      return component;
    }

    ++components_index;
    if (components_index == components.size()) {
      components_index = 0;
    }
  }
}

clip_player_uncompressed::clip_player_uncompressed(std::span<const uint32_t> data,
                                                   const float duration_s,
                                                   const skeleton& skeleton)
    : clip_player_base(duration_s), _data{data}, _skeleton{skeleton}
{
  // Retrieve which joints are used with what transform components
  // and prepare cursor for this data

  // Joint index -> transform components that are in animation
  // TODO: should this be stored in a clip instead?
  std::map<gsl::index, int> joints;

  gsl::index joint_index{std::numeric_limits<gsl::index>::max()};

  for (gsl::index data_pos{0}; data_pos < _data.size();) {
    const uint32_t key_header{data[data_pos]};

    if (has_flag(key_header, compression_key_flags::has_joint_index)) {
      joint_index = key_header >> 16;
    }

    gsl::index offset{1};  // 1 for header we already read

    if (has_flag(key_header, compression_key_flags::has_time)) {
      ++offset;
    }

    if (has_flag(key_header, compression_key_flags::has_translation)) {
      joints[joint_index] |= transform_components::translation;
      offset += 3;
    }

    if (has_flag(key_header, compression_key_flags::has_rotation)) {
      joints[joint_index] |= transform_components::rotation;
      offset += 4;
    }

    if (has_flag(key_header, compression_key_flags::has_scale)) {
      joints[joint_index] |= transform_components::scale;
      offset += 3;
    }

    data_pos += offset;
  }

  for (const auto& [index, components] : joints) {
    Expects(components != 0);

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

void clip_player_uncompressed::play_impl(const float time_s, skeleton_pose& out_pose)
{
  using flags = compression_key_flags;

  if (time_s < get_last_play_time_s()) {
    // Cursor cannot move backwards, need to start over
    cursor_reset(_cursor);
  }

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

    Expects(has_translation || has_rotation || has_scale);

    if (has_joint_index) {
      _cursor.last_data_joint_index = header >> 16;
    }

    cursor_component<float3>* translation{nullptr};
    cursor_component<quaternion>* rotation{nullptr};
    cursor_component<float3>* scale{nullptr};

    bool outdated{false};

    if (has_translation) {
      translation = &get_cursor_component(_cursor.translations, cursor_translation_index,
                                          _cursor.last_data_joint_index);
      outdated |= cursor_component_is_outdated(*translation, time_s);
    }

    if (has_rotation) {
      rotation = &get_cursor_component(_cursor.rotations, cursor_rotation_index,
                                       _cursor.last_data_joint_index);
      outdated |= cursor_component_is_outdated(*rotation, time_s);
    }

    if (has_scale) {
      scale =
          &get_cursor_component(_cursor.scales, cursor_scale_index, _cursor.last_data_joint_index);
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