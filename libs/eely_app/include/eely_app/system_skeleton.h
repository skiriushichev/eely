#pragma once

#include "eely_app/app.h"

#include <entt/entity/registry.hpp>

namespace eely {
void system_skeleton_update(app& app, entt::registry& registry, float dt_s);
}  // namespace eely