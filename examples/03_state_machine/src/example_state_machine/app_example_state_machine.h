#pragma once

#include <eely_app/anim_graph_editor.h>
#include <eely_app/app.h>
#include <eely_app/scene.h>

#include <eely/params/params.h>
#include <eely/project/project.h>

#include <entt/entt.hpp>

#include <memory>

namespace eely {
class app_example_state_machine final : public app {
public:
  app_example_state_machine(unsigned int width, unsigned int height, const std::string& title);

  void update(float dt_s) override;

private:
  std::unique_ptr<project> _project;
  scene _scene;
  entt::entity _character;
  params _params;
  bool _show_graph_editor{false};
  std::unique_ptr<anim_graph_editor> _anim_graph_editor;
};
}  // namespace eely