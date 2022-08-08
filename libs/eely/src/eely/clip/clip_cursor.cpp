#include "eely/clip/clip_cursor.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_pose.h"

#include <gsl/util>

#include <optional>
#include <vector>

namespace eely::internal {
void joint_components_collect(const std::vector<clip_uncooked::track>& tracks,
                              const skeleton& skeleton,
                              std::vector<joint_components>& out_joints_components)
{
  out_joints_components.reserve(tracks.size());

  for (const clip_uncooked::track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    const gsl::index joint_index{joint_index_opt.value()};

    int components{0};
    for (const auto& [_, key] : track.keys) {
      if (key.translation.has_value()) {
        components |= transform_components::translation;
      }

      if (key.rotation.has_value()) {
        components |= transform_components::rotation;
      }

      if (key.scale.has_value()) {
        components |= transform_components::scale;
      }
    }

    out_joints_components.push_back({.joint_index = joint_index, .components = components});
  }

  std::sort(out_joints_components.begin(), out_joints_components.end(),
            [](const auto& a, const auto& b) { return a.joint_index < b.joint_index; });
}

void cursor_init(cursor& cursor, const std::vector<joint_components>& joints_components)
{
  EXPECTS(!joints_components.empty());
  EXPECTS(cursor.translations.empty());
  EXPECTS(cursor.rotations.empty());
  EXPECTS(cursor.scales.empty());

  cursor.shallow_joint_index = joints_components.front().joint_index;

  for (const auto& [index, components] : joints_components) {
    EXPECTS(components != 0);

    if (has_flag(components, transform_components::translation)) {
      cursor_component<float3> component;
      component.joint_index = index;
      cursor.translations.push_back(component);
    }

    if (has_flag(components, transform_components::rotation)) {
      cursor_component<quaternion> component;
      component.joint_index = index;
      cursor.rotations.push_back(component);
    }

    if (has_flag(components, transform_components::scale)) {
      cursor_component<float3> component;
      component.joint_index = index;
      cursor.scales.push_back(component);
    }
  }
}

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
}  // namespace eely::internal