#include "eely_app/system_skeleton.h"

#include "eely_app/component_anim_graph.h"
#include "eely_app/component_clip.h"
#include "eely_app/component_skeleton.h"

#include <entt/entity/registry.hpp>

namespace eely {
void system_skeleton_update(app& /*app*/, entt::registry& registry, const float dt_s)
{
  auto clips_view{registry.view<component_skeleton, component_clip>()};
  for (entt::entity entity : clips_view) {
    component_clip& component_clip{clips_view.get<eely::component_clip>(entity)};
    if (component_clip.player == nullptr) {
      continue;
    }

    component_skeleton& component_skeleton{clips_view.get<eely::component_skeleton>(entity)};

    if (component_clip.play_time_s > component_clip.player->get_duration_s()) {
      component_clip.play_time_s = 0.0F;
    }

    component_clip.player->play(component_clip.play_time_s, component_skeleton.pose);

    component_clip.play_time_s += dt_s * component_clip.speed;
  }

  auto anim_graphs_view{registry.view<component_skeleton, component_anim_graph>()};
  for (entt::entity entity : anim_graphs_view) {
    component_anim_graph& component_anim_graph{
        anim_graphs_view.get<eely::component_anim_graph>(entity)};
    if (component_anim_graph.player == nullptr) {
      continue;
    }

    component_skeleton& component_skeleton{anim_graphs_view.get<eely::component_skeleton>(entity)};

    component_anim_graph.player->play(dt_s, component_skeleton.pose);
  }
}
}  // namespace eely