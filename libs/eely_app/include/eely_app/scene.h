#pragma once

#include "eely_app/app.h"

#include <entt/entity/registry.hpp>

#include <vector>

namespace eely {
// Represents ECS scene.
class scene final {
public:
  using system = void (*)(app& /* application */,
                          entt::registry& /* registry */,
                          float /* dt_s */);

  scene(app& app);

  // Return ECS registry.
  [[nodiscard]] entt::registry& get_registry();

  // Add system to be invoked every update.
  void add_system(system s);

  // Update the scene. This is where all systems are run.
  void update(float dt_s);

private:
  app& _app;
  entt::registry _entt_registry;
  std::vector<system> _systems;
};
}  // namespace eely