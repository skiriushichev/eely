#include "eely_app/scene.h"

namespace eely {
scene::scene(app& app) : _app{app} {}

entt::registry& scene::get_registry()
{
  return _entt_registry;
}

void scene::add_system(system s)
{
  _systems.push_back(s);
}

void scene::update(float dt_s)
{
  for (system s : _systems) {
    s(_app, _entt_registry, dt_s);
  }
}
}  // namespace eely