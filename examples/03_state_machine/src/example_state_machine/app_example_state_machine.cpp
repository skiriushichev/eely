#include "example_state_machine/app_example_state_machine.h"

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
#include <eely/anim_graph/anim_graph_node_and.h>
#include <eely/anim_graph/anim_graph_node_clip.h>
#include <eely/anim_graph/anim_graph_node_param.h>
#include <eely/anim_graph/anim_graph_node_param_comparison.h>
#include <eely/anim_graph/anim_graph_node_random.h>
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

static const string_id param_id_state{"state"};
static const string_id param_id_playback_speed{"playback_speed"};

enum class state { idle, block, slash, cast };

static std::unique_ptr<project> import_and_cook_resources()
{
  using namespace eely::internal;

  // Create or import resources from FBX (a skeleton, animation clips, a skeleton mask and a blend
  // tree) into uncooked project and cook into a runtime project.
  // This can also be done offline in the editor.

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};

  const std::filesystem::path exec_dir{get_executable_dir()};

  // Skeleton

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/stand.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/block_idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/cast_0.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/cast_1.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/idle_flex.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/block_start.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/slash_0.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/slash_1.fbx"}.import_clip(0, skeleton_uncooked);

  // Animation graph with a state machine node,
  // with according states and transitions between them.
  // Each state either plays a single clip or chooses randomy between two available clips:
  // there are two animations for slashing and casting.
  // Additional node controls playback speed of this whole tree.

  auto graph{std::make_unique<anim_graph_uncooked>("state_machine")};

  graph->set_skeleton_id(skeleton_uncooked.get_id());

  // Clips

  auto& node_idle{graph->add_node<anim_graph_node_clip>()};
  node_idle.set_clip_id("idle");

  auto& node_block_start{graph->add_node<anim_graph_node_clip>()};
  node_block_start.set_clip_id("block_start");

  auto& node_block_idle{graph->add_node<anim_graph_node_clip>()};
  node_block_idle.set_clip_id("block_idle");

  auto& node_slash0{graph->add_node<anim_graph_node_clip>()};
  node_slash0.set_clip_id("slash_0");

  auto& node_slash1{graph->add_node<anim_graph_node_clip>()};
  node_slash1.set_clip_id("slash_1");

  auto& node_slash{graph->add_node<anim_graph_node_random>()};
  node_slash.get_children_nodes() = {node_slash0.get_id(), node_slash1.get_id()};

  auto& node_cast0{graph->add_node<anim_graph_node_clip>()};
  node_cast0.set_clip_id("cast_0");

  auto& node_cast1{graph->add_node<anim_graph_node_clip>()};
  node_cast1.set_clip_id("cast_1");

  auto& node_cast{graph->add_node<anim_graph_node_random>()};
  node_cast.get_children_nodes() = {node_cast0.get_id(), node_cast1.get_id()};

  // States

  auto& node_state_idle{graph->add_node<anim_graph_node_state>()};
  node_state_idle.set_pose_node(node_idle.get_id());

  auto& node_state_block_start{graph->add_node<anim_graph_node_state>()};
  node_state_block_start.set_pose_node(node_block_start.get_id());

  auto& node_state_block_idle{graph->add_node<anim_graph_node_state>()};
  node_state_block_idle.set_pose_node(node_block_idle.get_id());

  auto& node_state_slash{graph->add_node<anim_graph_node_state>()};
  node_state_slash.set_pose_node(node_slash.get_id());

  auto& node_state_cast{graph->add_node<anim_graph_node_state>()};
  node_state_cast.set_pose_node(node_cast.get_id());

  // Conditions for transitions between states

  auto& node_condition_block_ended{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_block_ended.set_param_id(param_id_state);
  node_condition_block_ended.set_value(static_cast<int>(state::block));
  node_condition_block_ended.set_op(anim_graph_node_param_comparison::op::not_equal);

  auto& node_condition_block_start_ended{graph->add_node<anim_graph_node_state_condition>()};
  node_condition_block_start_ended.set_phase(1.0F);

  auto& node_condition_state_is_block{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_block.set_param_id(param_id_state);
  node_condition_state_is_block.set_value(static_cast<int>(state::block));

  auto& node_condition_state_is_slash{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_slash.set_param_id(param_id_state);
  node_condition_state_is_slash.set_value(static_cast<int>(state::slash));

  auto& node_condition_slash_animation_ended{graph->add_node<anim_graph_node_state_condition>()};
  node_condition_slash_animation_ended.set_phase(1.0F);

  auto& node_condition_slash_state_ended{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_slash_state_ended.set_param_id(param_id_state);
  node_condition_slash_state_ended.set_value(static_cast<int>(state::slash));
  node_condition_slash_state_ended.set_op(anim_graph_node_param_comparison::op::not_equal);

  auto& node_condition_slash_ended{graph->add_node<anim_graph_node_and>()};
  node_condition_slash_ended.get_children_nodes() = {node_condition_slash_animation_ended.get_id(),
                                                     node_condition_slash_state_ended.get_id()};

  auto& node_condition_state_is_cast{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_cast.set_param_id(param_id_state);
  node_condition_state_is_cast.set_value(static_cast<int>(state::cast));

  auto& node_condition_cast_animation_ended{graph->add_node<anim_graph_node_state_condition>()};
  node_condition_cast_animation_ended.set_phase(1.0F);

  auto& node_condition_cast_state_ended{graph->add_node<anim_graph_node_param_comparison>()};
  node_condition_cast_state_ended.set_param_id(param_id_state);
  node_condition_cast_state_ended.set_value(static_cast<int>(state::cast));
  node_condition_cast_state_ended.set_op(anim_graph_node_param_comparison::op::not_equal);

  auto& node_condition_cast_ended{graph->add_node<anim_graph_node_and>()};
  node_condition_cast_ended.get_children_nodes() = {node_condition_cast_animation_ended.get_id(),
                                                    node_condition_cast_state_ended.get_id()};
  // Transitions

  auto& node_transition_idle_to_block_start{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_block_start.set_condition_node(node_condition_state_is_block.get_id());
  node_transition_idle_to_block_start.set_destination_state_node(node_state_block_start.get_id());
  node_transition_idle_to_block_start.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(
      node_transition_idle_to_block_start.get_id());

  auto& node_transition_block_start_to_block_idle{
      graph->add_node<anim_graph_node_state_transition>()};
  node_transition_block_start_to_block_idle.set_condition_node(
      node_condition_block_start_ended.get_id());
  node_transition_block_start_to_block_idle.set_destination_state_node(
      node_state_block_idle.get_id());
  node_transition_block_start_to_block_idle.set_duration_s(0.1F);
  node_state_block_start.get_out_transition_nodes().push_back(
      node_transition_block_start_to_block_idle.get_id());

  auto& node_transition_block_to_idle{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_block_to_idle.set_condition_node(node_condition_block_ended.get_id());
  node_transition_block_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_block_to_idle.set_duration_s(0.2F);
  node_state_block_idle.get_out_transition_nodes().push_back(
      node_transition_block_to_idle.get_id());

  auto& node_transition_idle_to_slash{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_slash.set_condition_node(node_condition_state_is_slash.get_id());
  node_transition_idle_to_slash.set_destination_state_node(node_state_slash.get_id());
  node_transition_idle_to_slash.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_slash.get_id());

  auto& node_transition_slash_to_idle{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_slash_to_idle.set_condition_node(node_condition_slash_ended.get_id());
  node_transition_slash_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_slash_to_idle.set_duration_s(0.1F);
  node_state_slash.get_out_transition_nodes().push_back(node_transition_slash_to_idle.get_id());

  auto& node_transition_idle_to_cast{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_cast.set_condition_node(node_condition_state_is_cast.get_id());
  node_transition_idle_to_cast.set_destination_state_node(node_state_cast.get_id());
  node_transition_idle_to_cast.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_cast.get_id());

  auto& node_transition_cast_to_idle{graph->add_node<anim_graph_node_state_transition>()};
  node_transition_cast_to_idle.set_condition_node(node_condition_cast_ended.get_id());
  node_transition_cast_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_cast_to_idle.set_duration_s(0.1F);
  node_state_cast.get_out_transition_nodes().push_back(node_transition_cast_to_idle.get_id());

  // Speed node and input param

  auto& node_speed_param{graph->add_node<anim_graph_node_param>()};
  node_speed_param.set_param_id(param_id_playback_speed);

  auto& node_speed{graph->add_node<anim_graph_node_speed>()};
  node_speed.set_speed_provider_node(node_speed_param.get_id());

  // Bind all together

  auto& node_state_machine{graph->add_node<anim_graph_node_state_machine>()};
  node_state_machine.get_state_nodes() = {node_state_idle.get_id(), node_state_block_start.get_id(),
                                          node_state_block_idle.get_id()};

  node_speed.set_child_node(node_state_machine.get_id());

  graph->set_root_node_id(node_speed.get_id());

  project_uncooked.set_resource(std::move(graph));

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_state_machine::app_example_state_machine(const unsigned int width,
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
  registry.emplace<component_anim_graph>(_character, std::make_unique<anim_graph_player>(graph),
                                         &_params);

  // Initialize default parameter values

  _params.get_value<int>(param_id_state) = static_cast<int>(state::idle);
  _params.get_value<float>(param_id_playback_speed) = 1.0F;
}

void app_example_state_machine::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(340.0F, 80.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "State machine", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    int& state{_params.get_value<int>(param_id_state)};
    ImGui::Text("State: ");
    ImGui::SameLine();
    ImGui::RadioButton("Idle", &state, static_cast<int>(state::idle));
    ImGui::SameLine();
    ImGui::RadioButton("Block", &state, static_cast<int>(state::block));
    ImGui::SameLine();
    ImGui::RadioButton("Slash", &state, static_cast<int>(state::slash));
    ImGui::SameLine();
    ImGui::RadioButton("Cast", &state, static_cast<int>(state::cast));

    float& playback_speed{_params.get_value<float>(param_id_playback_speed)};
    ImGui::SliderFloat("Playback speed", &playback_speed, 0.0F, 2.0F, "%.1f");

    ImGui::End();
  }

  _scene.update(dt_s);
}
}  // namespace eely