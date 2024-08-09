#include "example_blend/app_example_blend.h"

#include <eely_app/app.h>
#include <eely_app/component_anim_graph.h>
#include <eely_app/component_camera.h>
#include <eely_app/component_skeleton.h>
#include <eely_app/component_transform.h>
#include <eely_app/filesystem_utils.h>
#include <eely_app/scene.h>
#include <eely_app/system_camera.h>
#include <eely_app/system_render.h>
#include <eely_app/system_skeleton.h>

#include <eely_importer/importer.h>

#include <eely/anim_graph/anim_graph.h>
#include <eely/anim_graph/anim_graph_node_blend.h>
#include <eely/anim_graph/anim_graph_node_clip.h>
#include <eely/anim_graph/anim_graph_node_param.h>
#include <eely/anim_graph/anim_graph_node_speed.h>
#include <eely/anim_graph/anim_graph_uncooked.h>
#include <eely/base/string_id.h>
#include <eely/math/float3.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_uncooked.h>

#include <bgfx/bgfx.h>

#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0x31363DFF};

static const string_id param_id_speed{"speed"};
static const string_id param_id_crouch{"crouch"};
static const string_id param_id_playback_speed{"playback_speed"};

static constexpr float param_speed_walk{1.0F};
static constexpr float param_speed_jog{2.0F};
static constexpr float param_speed_run{3.0F};

static std::unique_ptr<project> import_and_cook_resources()
{
  using namespace eely::internal;

  // Create or import resources from FBX (skeleton, clip, animation graph with blending)
  // into uncooked project and cook into a runtime project.
  // This can also be done offline in the editor.

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};

  const std::filesystem::path exec_dir{get_executable_dir()};

  // Skeleton

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/walk.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/jog.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/run_crouch.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/run.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walk_crouch.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walk.fbx"}.import_clip(0, skeleton_uncooked);

  // Blending animation graph:
  //  - three standing movement animations, blended by speed
  //  - two crouching movement animations, blended by speed
  //  - standing and movement animations are blended by crouching parameter
  // Additional node controls playback speed of this whole tree.

  auto& graph{project_uncooked.add_resource<anim_graph_uncooked>("graph")};

  graph.set_skeleton_id(skeleton_uncooked.get_id());

  // Clips

  auto& node_walk{graph.add_node<anim_graph_node_clip>()};
  node_walk.set_clip_id("walk");
  node_walk.set_editor_position(float3{692.0F, 97.0F, 0.0F});

  auto& node_jog{graph.add_node<anim_graph_node_clip>()};
  node_jog.set_clip_id("jog");
  node_jog.set_editor_position(float3{692.0F, 177.0F, 0.0F});

  auto& node_run{graph.add_node<anim_graph_node_clip>()};
  node_run.set_clip_id("run");
  node_run.set_editor_position(float3{692.0F, 257.0F, 0.0F});

  auto& node_crouch_walk{graph.add_node<anim_graph_node_clip>()};
  node_crouch_walk.set_clip_id("walk_crouch");
  node_crouch_walk.set_editor_position(float3{692.0F, 474.0F, 0.0F});

  auto& node_crouch_run{graph.add_node<anim_graph_node_clip>()};
  node_crouch_run.set_clip_id("run_crouch");
  node_crouch_run.set_editor_position(float3{692.0F, 554.0F, 0.0F});

  // Params

  auto& node_speed_param_for_walk{graph.add_node<anim_graph_node_param>()};
  node_speed_param_for_walk.set_param_id(param_id_speed);
  node_speed_param_for_walk.set_editor_position(float3{692.0F, -14.0F, 0.0F});

  auto& node_speed_param_for_crouch{graph.add_node<anim_graph_node_param>()};
  node_speed_param_for_crouch.set_param_id(param_id_speed);
  node_speed_param_for_crouch.set_editor_position(float3{692.0F, 369.0F, 0.0F});

  auto& node_crouch_param{graph.add_node<anim_graph_node_param>()};
  node_crouch_param.set_param_id(param_id_crouch);
  node_crouch_param.set_editor_position(float3{436.0F, 17.0F, 0.0F});

  auto& node_playback_speed_param{graph.add_node<anim_graph_node_param>()};
  node_playback_speed_param.set_param_id(param_id_playback_speed);
  node_playback_speed_param.set_editor_position(float3{228.0F, -46.0F, 0.0F});

  // Blends

  auto& node_blend_walk_jog_run{graph.add_node<anim_graph_node_blend>()};
  node_blend_walk_jog_run.get_pose_nodes() = {
      {.id = node_walk.get_id(), .factor = param_speed_walk},
      {.id = node_jog.get_id(), .factor = param_speed_jog},
      {.id = node_run.get_id(), .factor = param_speed_run},
  };
  node_blend_walk_jog_run.set_factor_node_id(node_speed_param_for_walk.get_id());
  node_blend_walk_jog_run.set_editor_position(float3{430.0F, 125.0F, 0.0F});

  auto& node_blend_crouch_walk_run{graph.add_node<anim_graph_node_blend>()};
  node_blend_crouch_walk_run.get_pose_nodes() = {
      {.id = node_crouch_walk.get_id(), .factor = param_speed_walk},
      {.id = node_crouch_run.get_id(), .factor = param_speed_run},
  };
  node_blend_crouch_walk_run.set_factor_node_id(node_speed_param_for_crouch.get_id());
  node_blend_crouch_walk_run.set_editor_position(float3{430.0F, 400.0F, 0.0F});

  auto& node_blend_stand_crouch{graph.add_node<anim_graph_node_blend>()};
  node_blend_stand_crouch.get_pose_nodes() = {
      {.id = node_blend_walk_jog_run.get_id(), .factor = 0.0F},
      {.id = node_blend_crouch_walk_run.get_id(), .factor = 1.0F},
  };
  node_blend_stand_crouch.set_factor_node_id(node_crouch_param.get_id());
  node_blend_stand_crouch.set_editor_position(float3{228.0F, 40.0F, 0.0F});

  // Playback speed node

  auto& node_playback_speed{graph.add_node<anim_graph_node_speed>()};
  node_playback_speed.set_speed_provider_node(node_playback_speed_param.get_id());
  node_playback_speed.set_child_node(node_blend_stand_crouch.get_id());
  node_playback_speed.set_editor_position(float3{0.0F, 0.0F, 0.0F});

  graph.set_root_node_id(node_playback_speed.get_id());

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 64)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_blend::app_example_blend(const unsigned int width,
                                     const unsigned int height,
                                     const std::string& title)
    : app(width, height, title), _project{import_and_cook_resources()}, _scene(*this)
{
  _scene.add_system(&system_skeleton_update);
  _scene.add_system(&system_camera_update);
  _scene.add_system(&system_render_update);

  entt::registry& registry{_scene.get_registry()};

  // Create camera

  entt::entity camera{registry.create()};
  registry.emplace<component_transform>(camera, transform{float3{0.0F, 0.75F, 3.0F}});
  component_camera& component_camera{registry.emplace<eely::component_camera>(camera)};
  component_camera.yaw = pi;

  // Create character

  const skeleton& skeleton{*_project->get_resource<eely::skeleton>("mixamorig:Hips")};
  const anim_graph& graph{*_project->get_resource<eely::anim_graph>("graph")};

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));
  auto& component_anim_graph{registry.emplace<eely::component_anim_graph>(
      _character, std::make_unique<anim_graph_player>(graph), &_params)};

  // Initialize default parameter values

  _params.get_value<float>(param_id_speed) = param_speed_walk;
  _params.get_value<float>(param_id_crouch) = 0.0F;
  _params.get_value<float>(param_id_playback_speed) = 1.0F;

  // Initialize graph editor for visualization

  _anim_graph_editor =
      std::make_unique<anim_graph_editor>(graph, component_anim_graph.player.get());
}

void app_example_blend::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, gsl::narrow<uint16_t>(get_width()),
                    gsl::narrow<uint16_t>(get_height()));
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(350.0F, 0.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "Blend", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    float& speed_value{_params.get_value<float>(param_id_speed)};
    ImGui::SliderFloat("Speed", &speed_value, param_speed_walk, param_speed_run, "%.2f");

    float& crouch_value{_params.get_value<float>(param_id_crouch)};
    ImGui::SliderFloat("Crouch", &crouch_value, 0.0F, 1.0F, "%.2f");

    float& playback_speed_value{_params.get_value<float>(param_id_playback_speed)};
    ImGui::SliderFloat("Playback speed", &playback_speed_value, 0.0F, 2.0F, "%.2f");

    ImGui::Separator();

    ImGui::Checkbox("Show animation graph", &_show_graph_editor);

    ImGui::End();
  }

  if (_show_graph_editor) {
    ImGui::SetNextWindowSize(ImVec2(600.0F, 600.0F), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(10.0F, 160.0F), ImGuiCond_Once);
    if (ImGui::Begin("Graph", nullptr, ImGuiWindowFlags_NoScrollbar)) {
      _anim_graph_editor->render();
      ImGui::End();
    }
  }

  _scene.update(dt_s);
}
}  // namespace eely