#pragma once

#include <eely_app/app.h>
#include <eely_app/scene.h>

#include <eely/btree/btree_player.h>
#include <eely/params/params.h>
#include <eely/project/project.h>

#include <entt/entt.hpp>

#include <memory>

namespace eely {
class app_example_btree final : public app {
public:
  app_example_btree(unsigned int width, unsigned int height, const std::string& title);

  void update(float dt_s) override;

private:
  std::unique_ptr<project> _project;
  scene _scene;
  entt::entity _character;

  params _params;
  std::unique_ptr<btree_player> _btree_player;
};
}  // namespace eely