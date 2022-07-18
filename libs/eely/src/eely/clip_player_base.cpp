#include "eely/clip_player_base.h"

#include "eely/float3.h"
#include "eely/quaternion.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"

#include <gsl/util>

#include <limits>
#include <vector>

namespace eely {
clip_player_base::clip_player_base(const float duration_s) : _duration_s(duration_s) {}

clip_player_base::~clip_player_base() = default;

void clip_player_base::play(const float time_s, skeleton_pose& out_pose)
{
  Expects(time_s >= 0.0F && time_s <= _duration_s);

  play_impl(time_s, out_pose);
  _last_play_time_s = time_s;
}

float clip_player_base::get_duration_s() const
{
  return _duration_s;
}

float clip_player_base::get_last_play_time_s() const
{
  return _last_play_time_s;
}

void cursor_reset(cursor& cursor)
{
  cursor.last_data_pos = 0;
  cursor.last_data_time_s = -1.0F;
  cursor.last_data_joint_index = std::numeric_limits<gsl::index>::max();

  for (auto& t : cursor.translations) {
    cursor_component_reset(t);
  }

  for (auto& r : cursor.rotations) {
    cursor_component_reset(r);
  }

  for (auto& s : cursor.scales) {
    cursor_component_reset(s);
  }
}

void cursor_calculate_pose(const cursor& cursor, const float time_s, skeleton_pose& out_pose)
{
  gsl::index translation_index{0};
  gsl::index rotation_index{0};
  gsl::index scale_index{0};

  while (true) {
    gsl::index joint_index{std::numeric_limits<gsl::index>::max()};

    const cursor_component<float3>* translation{nullptr};
    const cursor_component<quaternion>* rotation{nullptr};
    const cursor_component<float3>* scale{nullptr};

    if (translation_index < cursor.translations.size()) {
      translation = &cursor.translations[translation_index];
      joint_index = std::min(joint_index, translation->joint_index);
    }

    if (rotation_index < cursor.rotations.size()) {
      rotation = &cursor.rotations[rotation_index];
      joint_index = std::min(joint_index, rotation->joint_index);
    }

    if (scale_index < cursor.scales.size()) {
      scale = &cursor.scales[scale_index];
      joint_index = std::min(joint_index, scale->joint_index);
    }

    if (joint_index == std::numeric_limits<gsl::index>::max()) {
      break;
    }

    Expects(translation != nullptr || rotation != nullptr || scale != nullptr);

    transform joint_transform{out_pose.get_transform_joint_space(joint_index)};

    if (translation != nullptr && translation->joint_index == joint_index) {
      ++translation_index;
      joint_transform.translation = cursor_component_calculate(*translation, time_s);
    }

    if (rotation != nullptr && rotation->joint_index == joint_index) {
      ++rotation_index;
      joint_transform.rotation = cursor_component_calculate(*rotation, time_s);
    }

    if (scale != nullptr && scale->joint_index == joint_index) {
      ++scale_index;
      joint_transform.scale = cursor_component_calculate(*scale, time_s);
    }

    out_pose.set_transform_joint_space(joint_index, joint_transform);
  }
}
}  // namespace eely