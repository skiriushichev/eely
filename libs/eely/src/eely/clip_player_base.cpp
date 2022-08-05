#include "eely/clip_player_base.h"

#include "eely/assert.h"
#include "eely/float3.h"
#include "eely/quaternion.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"

#include <gsl/util>

#include <limits>
#include <vector>

namespace eely {
clip_player_base::~clip_player_base() = default;

void cursor_reset(cursor& cursor)
{
  cursor.last_play_time_s = -1.0F;
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
  // Go over all components and set each one.
  // Even though it means we're moving across transforms array up to 3 times,
  // in practices we're dealing with rotations most of the times,
  // and very small number of translations and scales.

  out_pose.sequence_start(cursor.shallow_joint_index);

  const gsl::index rotations_count{gsl::narrow_cast<gsl::index>(cursor.rotations.size())};
  for (const auto& rotation_component : cursor.rotations) {
    const quaternion rotation{cursor_component_calculate(rotation_component, time_s)};
    out_pose.sequence_set_rotation_joint_space(rotation_component.joint_index, rotation);
  }

  const gsl::index translations_count{gsl::narrow_cast<gsl::index>(cursor.translations.size())};
  for (const auto& translation_component : cursor.translations) {
    const float3 translation{cursor_component_calculate(translation_component, time_s)};
    out_pose.sequence_set_translation_joint_space(translation_component.joint_index, translation);
  }

  const gsl::index scales_count{gsl::narrow_cast<gsl::index>(cursor.scales.size())};
  for (const auto& scale_component : cursor.scales) {
    const float3 scale{cursor_component_calculate(scale_component, time_s)};
    out_pose.sequence_set_scale_joint_space(scale_component.joint_index, scale);
  }
}
}  // namespace eely