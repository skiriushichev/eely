#pragma once

#include <eely_app/app.h>
#include <eely_app/scene.h>

#include <eely/project/project.h>

#include <entt/entt.hpp>

namespace eely {
class app_example_clip final : public app {
public:
  app_example_clip(unsigned int width, unsigned int height, const std::string& title);

  void update(float dt_s) override;

private:
  std::unique_ptr<project> _project;
  scene _scene;
  entt::entity _character;
};
}  // namespace eely