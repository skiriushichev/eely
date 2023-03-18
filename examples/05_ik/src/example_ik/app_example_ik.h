#pragma once

#include <eely_app/app.h>
#include <eely_app/scene.h>

#include <eely/project/project.h>

#include <entt/entt.hpp>

namespace eely {
class app_example_ik final : public app {
public:
  app_example_ik(unsigned int width, unsigned int height, const std::string& title);

  void update(float dt_s) override;

private:
  enum class mode { ik, constraints };

  std::unique_ptr<project> _project;
  scene _scene;
  entt::entity _character;
  entt::entity _character_unconstrained;
  mode _mode{mode::constraints};

  // Constraints mode
  string_id _constraints_selected_joint_id;
  bool _constraints_render_joint_frame{true};
  bool _constraints_render_parent_constraint_frame{false};
  bool _constraints_render_child_constraint_frame{false};
  bool _constraints_render_limits{false};
};
}  // namespace eely