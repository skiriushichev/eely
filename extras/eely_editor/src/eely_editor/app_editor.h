#pragma once

#include <eely_app/app.h>
#include <eely_app/scene.h>

#include <eely/project.h>

#include <entt/entt.hpp>

namespace eely {
class app_editor final : public app {
public:
  app_editor(unsigned int width, unsigned int height, const std::string& title);

  void update(float dt_s) override;

private:
  scene _scene;
};
}  // namespace eely