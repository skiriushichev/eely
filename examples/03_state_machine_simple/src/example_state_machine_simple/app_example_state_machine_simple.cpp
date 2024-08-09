#include "example_state_machine_simple/app_example_state_machine_simple.h"

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
#include <eely/anim_graph/anim_graph_node_clip.h>
#include <eely/anim_graph/anim_graph_node_param.h>
#include <eely/anim_graph/anim_graph_node_param_comparison.h>
#include <eely/anim_graph/anim_graph_node_speed.h>
#include <eely/anim_graph/anim_graph_node_state.h>
#include <eely/anim_graph/anim_graph_node_state_condition.h>
#include <eely/anim_graph/anim_graph_node_state_machine.h>
#include <eely/anim_graph/anim_graph_node_state_transition.h>
#include <eely/anim_graph/anim_graph_uncooked.h>
#include <eely/clip/clip.h>
#include <eely/clip/clip_uncooked.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_uncooked.h>

#include <entt/entt.hpp>

#include <gsl/util>

#include <imgui.h>

#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0x31363DFF};

static const string_id param_id_trigger_taunt{"taunt"};
static const string_id param_id_playback_speed{"playback_speed"};

static std::unique_ptr<project> import_and_cook_resources()
{
  using namespace eely::internal;

  // Create or import resources from FBX (a skeleton, animation clips, a skeleton mask and a blend
  // tree) into uncooked project and cook into a runtime project.
  // This can also be done offline in the editor.

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};

  const std::filesystem::path exec_dir{get_executable_dir()};

  // Skeleton

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/idle.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/taunt.fbx"}.import_clip(0, skeleton_uncooked);

  // Animation graph with a state machine node,
  // with according states and transitions between them.
  // Three states: idle, combo & taunt with accoridng transitions.
  // Each state just plays a single clip.

  auto& graph{project_uncooked.add_resource<anim_graph_uncooked>("state_machine")};

  graph.set_skeleton_id(skeleton_uncooked.get_id());

  // Clips

  auto& node_idle{graph.add_node<anim_graph_node_clip>()};
  node_idle.set_clip_id("idle");
  node_idle.set_editor_position(float3{616.0F, -81.0F, 0.0F});

  auto& node_taunt{graph.add_node<anim_graph_node_clip>()};
  node_taunt.set_clip_id("taunt");
  node_taunt.set_editor_position(float3{616.0F, 238.0F, 0.0F});

  // State machine

  auto& node_state_idle{graph.add_node<anim_graph_node_state>()};
  node_state_idle.set_pose_node(node_idle.get_id());
  node_state_idle.set_name("idle");
  node_state_idle.set_editor_position(float3{440.0F, 12.0F, 0.0F});

  auto& node_state_taunt{graph.add_node<anim_graph_node_state>()};
  node_state_taunt.set_pose_node(node_taunt.get_id());
  node_state_taunt.set_name("taunt");
  node_state_taunt.set_editor_position(float3{440.0F, 119.0F, 0.0F});

  auto& node_condition_taunt_requested{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_taunt_requested.set_param_id(param_id_trigger_taunt);
  node_condition_taunt_requested.set_value(true);
  node_condition_taunt_requested.set_editor_position(float3{1099.0F, -9.0F, 0.0F});

  auto& node_condition_taunt_ended{graph.add_node<anim_graph_node_state_condition>()};
  node_condition_taunt_ended.set_phase(1.0F);
  node_condition_taunt_ended.set_editor_position(float3{1104.0F, 166.0F, 0.0F});

  auto& node_transition_idle_to_taunt{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_taunt.set_condition_node(node_condition_taunt_requested.get_id());
  node_transition_idle_to_taunt.set_destination_state_node(node_state_taunt.get_id());
  node_transition_idle_to_taunt.set_duration_s(0.1F);
  node_transition_idle_to_taunt.set_reversible(false);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_taunt.get_id());
  node_transition_idle_to_taunt.set_editor_position(float3{792.0F, 17.0F, 0.0F});

  auto& node_transition_taunt_to_idle{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_taunt_to_idle.set_condition_node(node_condition_taunt_ended.get_id());
  node_transition_taunt_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_taunt_to_idle.set_duration_s(0.2F);
  node_state_taunt.get_out_transition_nodes().push_back(node_transition_taunt_to_idle.get_id());
  node_transition_taunt_to_idle.set_editor_position(float3{792.0F, 145.0F, 0.0F});

  auto& node_state_machine{graph.add_node<anim_graph_node_state_machine>()};
  node_state_machine.get_state_nodes() = {node_state_idle.get_id(), node_state_taunt.get_id()};
  node_state_machine.set_editor_position(float3{226.0F, 46.0F, 0.0F});

  // Playback speed node and input param

  auto& node_speed_param{graph.add_node<anim_graph_node_param>()};
  node_speed_param.set_param_id(param_id_playback_speed);
  node_speed_param.set_editor_position(float3{232.0F, -57.0F, 0.0F});

  auto& node_speed{graph.add_node<anim_graph_node_speed>()};
  node_speed.set_speed_provider_node(node_speed_param.get_id());
  node_speed.set_editor_position(float3{2.0F, 4.0F, 0.0F});
  node_speed.set_child_node(node_state_machine.get_id());

  graph.set_root_node_id(node_speed.get_id());

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_state_machine_simple::app_example_state_machine_simple(const unsigned int width,
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
  const anim_graph& graph{*_project->get_resource<eely::anim_graph>("state_machine")};

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));
  auto& component_anim_graph{registry.emplace<eely::component_anim_graph>(
      _character, std::make_unique<anim_graph_player>(graph), &_params)};

  // Initialize default parameter values

  _params.get_value<float>(param_id_playback_speed) = 1.0F;

  // Initialize graph editor for visualization

  _anim_graph_editor =
      std::make_unique<anim_graph_editor>(graph, component_anim_graph.player.get());
}

void app_example_state_machine_simple::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, gsl::narrow<uint16_t>(get_width()),
                    gsl::narrow<uint16_t>(get_height()));
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(350.0F, 0.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "State machine", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    _params.get_value<bool>(param_id_trigger_taunt) = ImGui::Button("Taunt");

    float& playback_speed{_params.get_value<float>(param_id_playback_speed)};
    ImGui::SliderFloat("Playback speed", &playback_speed, 0.0F, 2.0F, "%.2f");

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