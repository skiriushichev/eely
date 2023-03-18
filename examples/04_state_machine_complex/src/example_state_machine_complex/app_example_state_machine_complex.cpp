#include "example_state_machine_complex/app_example_state_machine_complex.h"

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

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/idle.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/block_idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/block_start.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/cast_0.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/cast_1.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/slash_0.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/slash_1.fbx"}.import_clip(0, skeleton_uncooked);

  // Animation graph with a state machine node,
  // with according states and transitions between them.
  // Each state either:
  //  - plays a single clip
  //  - selects a random clip from two given (there are two slash and two cast clips)
  //  - or runs a nested state machine (block has a state machine from two states: start + idle)
  // Additional node controls playback speed of this whole tree.

  auto& graph{project_uncooked.add_resource<anim_graph_uncooked>("state_machine")};

  graph.set_skeleton_id(skeleton_uncooked.get_id());

  // Clips

  auto& node_idle{graph.add_node<anim_graph_node_clip>()};
  node_idle.set_clip_id("idle");
  node_idle.set_editor_position(float3{708.0F, -500.0F, 0.0F});

  auto& node_block_start{graph.add_node<anim_graph_node_clip>()};
  node_block_start.set_clip_id("block_start");
  node_block_start.set_editor_position(float3{1112.0F, -177.0F, 0.0F});

  auto& node_block_idle{graph.add_node<anim_graph_node_clip>()};
  node_block_idle.set_clip_id("block_idle");
  node_block_idle.set_editor_position(float3{1117.0F, 94.0F, 0.0F});

  auto& node_slash0{graph.add_node<anim_graph_node_clip>()};
  node_slash0.set_clip_id("slash_0");
  node_slash0.set_editor_position(float3{914.0F, 200.0F, 0.0F});

  auto& node_slash1{graph.add_node<anim_graph_node_clip>()};
  node_slash1.set_clip_id("slash_1");
  node_slash1.set_editor_position(float3{914.0F, 278.0F, 0.0F});

  auto& node_slash{graph.add_node<anim_graph_node_random>()};
  node_slash.get_children_nodes() = {node_slash0.get_id(), node_slash1.get_id()};
  node_slash.set_editor_position(float3{706.0F, 224.0F, 0.0F});

  auto& node_cast0{graph.add_node<anim_graph_node_clip>()};
  node_cast0.set_clip_id("cast_0");
  node_cast0.set_editor_position(float3{946.0F, 522.0F, 0.0F});

  auto& node_cast1{graph.add_node<anim_graph_node_clip>()};
  node_cast1.set_clip_id("cast_1");
  node_cast1.set_editor_position(float3{946.0F, 602.0F, 0.0F});

  auto& node_cast{graph.add_node<anim_graph_node_random>()};
  node_cast.get_children_nodes() = {node_cast0.get_id(), node_cast1.get_id()};
  node_cast.set_editor_position(float3{738.0F, 541.0F, 0.0F});

  // Nested state machine for block (start + idle states)

  auto& node_state_block_start{graph.add_node<anim_graph_node_state>()};
  node_state_block_start.set_pose_node(node_block_start.get_id());
  node_state_block_start.set_name("block start");
  node_state_block_start.set_editor_position(float3{904.0F, -99.0F, 0.0F});

  auto& node_state_block_idle{graph.add_node<anim_graph_node_state>()};
  node_state_block_idle.set_pose_node(node_block_idle.get_id());
  node_state_block_idle.set_name("block idle");
  node_state_block_idle.set_editor_position(float3{908.0F, 0.0F, 0.0F});

  auto& node_condition_block_start_ended{graph.add_node<anim_graph_node_state_condition>()};
  node_condition_block_start_ended.set_phase(1.0F);
  node_condition_block_start_ended.set_editor_position(float3{1544.0F, -25.0F, 0.0F});

  auto& node_transition_block_start_to_block_idle{
      graph.add_node<anim_graph_node_state_transition>()};
  node_transition_block_start_to_block_idle.set_condition_node(
      node_condition_block_start_ended.get_id());
  node_transition_block_start_to_block_idle.set_destination_state_node(
      node_state_block_idle.get_id());
  node_transition_block_start_to_block_idle.set_duration_s(0.1F);
  node_state_block_start.get_out_transition_nodes().push_back(
      node_transition_block_start_to_block_idle.get_id());
  node_transition_block_start_to_block_idle.set_editor_position(float3{1160.0F, -46.0F, 0.0F});

  auto& node_block_state_machine{graph.add_node<anim_graph_node_state_machine>()};
  node_block_state_machine.get_state_nodes() = {node_state_block_start.get_id(),
                                                node_state_block_idle.get_id()};
  node_block_state_machine.set_editor_position(float3{706.0F, -52.0F, 0.0F});

  // Main state machine

  auto& node_state_idle{graph.add_node<anim_graph_node_state>()};
  node_state_idle.set_pose_node(node_idle.get_id());
  node_state_idle.set_name("idle");
  node_state_idle.set_editor_position(float3{520.0F, -417.0F, 0.0F});

  auto& node_state_block{graph.add_node<anim_graph_node_state>()};
  node_state_block.set_pose_node(node_block_state_machine.get_id());
  node_state_block.set_name("block");
  node_state_block.set_editor_position(float3{525.0F, -52.0F, 0.0F});

  auto& node_state_slash{graph.add_node<anim_graph_node_state>()};
  node_state_slash.set_pose_node(node_slash.get_id());
  node_state_slash.set_name("slash");
  node_state_slash.set_editor_position(float3{525.0F, 224.0F, 0.0F});

  auto& node_state_cast{graph.add_node<anim_graph_node_state>()};
  node_state_cast.set_pose_node(node_cast.get_id());
  node_state_cast.set_name("cast");
  node_state_cast.set_editor_position(float3{525.0F, 541.0F, 0.0F});

  auto& node_condition_block_ended{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_block_ended.set_param_id(param_id_state);
  node_condition_block_ended.set_value(static_cast<int>(state::block));
  node_condition_block_ended.set_op(anim_graph_node_param_comparison::op::not_equal);
  node_condition_block_ended.set_editor_position(float3{2110.0F, -89.0F, 0.0F});

  auto& node_condition_state_is_block{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_block.set_param_id(param_id_state);
  node_condition_state_is_block.set_value(static_cast<int>(state::block));
  node_condition_state_is_block.set_editor_position(float3{1342.0F, -633.0F, 0.0F});

  auto& node_condition_state_is_slash{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_slash.set_param_id(param_id_state);
  node_condition_state_is_slash.set_value(static_cast<int>(state::slash));
  node_condition_state_is_slash.set_editor_position(float3{1342.0F, -489.0F, 0.0F});

  auto& node_condition_slash_animation_ended{graph.add_node<anim_graph_node_state_condition>()};
  node_condition_slash_animation_ended.set_phase(1.0F);
  node_condition_slash_animation_ended.set_editor_position(float3{1706.0F, 214.0F, 0.0F});

  auto& node_condition_slash_state_ended{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_slash_state_ended.set_param_id(param_id_state);
  node_condition_slash_state_ended.set_value(static_cast<int>(state::slash));
  node_condition_slash_state_ended.set_op(anim_graph_node_param_comparison::op::not_equal);
  node_condition_slash_state_ended.set_editor_position(float3{1704.0F, 294.0F, 0.0F});

  auto& node_condition_slash_ended{graph.add_node<anim_graph_node_and>()};
  node_condition_slash_ended.get_children_nodes() = {node_condition_slash_animation_ended.get_id(),
                                                     node_condition_slash_state_ended.get_id()};
  node_condition_slash_ended.set_editor_position(float3{1492.0F, 215.0F, 0.0F});

  auto& node_condition_state_is_cast{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_state_is_cast.set_param_id(param_id_state);
  node_condition_state_is_cast.set_value(static_cast<int>(state::cast));
  node_condition_state_is_cast.set_editor_position(float3{1342.0F, -345.0F, 0.0F});

  auto& node_condition_cast_animation_ended{graph.add_node<anim_graph_node_state_condition>()};
  node_condition_cast_animation_ended.set_phase(1.0F);
  node_condition_cast_animation_ended.set_editor_position(float3{1712.0F, 550.0F, 0.0F});

  auto& node_condition_cast_state_ended{graph.add_node<anim_graph_node_param_comparison>()};
  node_condition_cast_state_ended.set_param_id(param_id_state);
  node_condition_cast_state_ended.set_value(static_cast<int>(state::cast));
  node_condition_cast_state_ended.set_op(anim_graph_node_param_comparison::op::not_equal);
  node_condition_cast_state_ended.set_editor_position(float3{1710.0F, 630.0F, 0.0F});

  auto& node_condition_cast_ended{graph.add_node<anim_graph_node_and>()};
  node_condition_cast_ended.get_children_nodes() = {node_condition_cast_animation_ended.get_id(),
                                                    node_condition_cast_state_ended.get_id()};
  node_condition_cast_ended.set_editor_position(float3{1506.0F, 566.0F, 0.0F});

  auto& node_transition_idle_to_block{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_block.set_condition_node(node_condition_state_is_block.get_id());
  node_transition_idle_to_block.set_destination_state_node(node_state_block.get_id());
  node_transition_idle_to_block.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_block.get_id());
  node_transition_idle_to_block.set_editor_position(float3{1024.0F, -622.0F, 0.0F});

  auto& node_transition_block_to_idle{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_block_to_idle.set_condition_node(node_condition_block_ended.get_id());
  node_transition_block_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_block_to_idle.set_duration_s(0.2F);
  node_state_block.get_out_transition_nodes().push_back(node_transition_block_to_idle.get_id());
  node_transition_block_to_idle.set_editor_position(float3{1770.0F, -65.0F, 0.0F});

  auto& node_transition_idle_to_slash{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_slash.set_condition_node(node_condition_state_is_slash.get_id());
  node_transition_idle_to_slash.set_destination_state_node(node_state_slash.get_id());
  node_transition_idle_to_slash.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_slash.get_id());
  node_transition_idle_to_slash.set_editor_position(float3{1028.0F, -481.0F, 0.0F});

  auto& node_transition_slash_to_idle{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_slash_to_idle.set_condition_node(node_condition_slash_ended.get_id());
  node_transition_slash_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_slash_to_idle.set_duration_s(0.1F);
  node_state_slash.get_out_transition_nodes().push_back(node_transition_slash_to_idle.get_id());
  node_transition_slash_to_idle.set_editor_position(float3{1192.0F, 209.0F, 0.0F});

  auto& node_transition_idle_to_cast{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_idle_to_cast.set_condition_node(node_condition_state_is_cast.get_id());
  node_transition_idle_to_cast.set_destination_state_node(node_state_cast.get_id());
  node_transition_idle_to_cast.set_duration_s(0.1F);
  node_state_idle.get_out_transition_nodes().push_back(node_transition_idle_to_cast.get_id());
  node_transition_idle_to_cast.set_editor_position(float3{1024.0F, -350.0F, 0.0F});

  auto& node_transition_cast_to_idle{graph.add_node<anim_graph_node_state_transition>()};
  node_transition_cast_to_idle.set_condition_node(node_condition_cast_ended.get_id());
  node_transition_cast_to_idle.set_destination_state_node(node_state_idle.get_id());
  node_transition_cast_to_idle.set_duration_s(0.1F);
  node_state_cast.get_out_transition_nodes().push_back(node_transition_cast_to_idle.get_id());
  node_transition_cast_to_idle.set_editor_position(float3{1192.0F, 545.0F, 0.0F});

  auto& node_state_machine{graph.add_node<anim_graph_node_state_machine>()};
  node_state_machine.get_state_nodes() = {node_state_idle.get_id(), node_state_block.get_id(),
                                          node_state_cast.get_id(), node_state_slash.get_id()};
  node_state_machine.set_editor_position(float3{264.0F, 46.0F, 0.0F});

  // Playback speed node and input param

  auto& node_speed_param{graph.add_node<anim_graph_node_param>()};
  node_speed_param.set_param_id(param_id_playback_speed);
  node_speed_param.set_editor_position(float3{264.0F, -41.0F, 0.0F});

  auto& node_speed{graph.add_node<anim_graph_node_speed>()};
  node_speed.set_speed_provider_node(node_speed_param.get_id());
  node_speed.set_editor_position(float3{0.0F, 0.0F, 0.0F});
  node_speed.set_child_node(node_state_machine.get_id());

  graph.set_root_node_id(node_speed.get_id());

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_state_machine_complex::app_example_state_machine_complex(const unsigned int width,
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

  _params.get_value<int>(param_id_state) = static_cast<int>(state::idle);
  _params.get_value<float>(param_id_playback_speed) = 1.0F;

  // Initialize graph editor for visualization

  _anim_graph_editor =
      std::make_unique<anim_graph_editor>(graph, component_anim_graph.player.get());
}

void app_example_state_machine_complex::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(350.0F, 0.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "State machine", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    int& state{_params.get_value<int>(param_id_state)};
    ImGui::TextUnformatted("State: ");
    ImGui::SameLine();
    ImGui::RadioButton("Idle", &state, static_cast<int>(state::idle));
    ImGui::SameLine();
    ImGui::RadioButton("Block", &state, static_cast<int>(state::block));
    ImGui::SameLine();
    ImGui::RadioButton("Slash", &state, static_cast<int>(state::slash));
    ImGui::SameLine();
    ImGui::RadioButton("Cast", &state, static_cast<int>(state::cast));

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