#include "example_additive/app_example_additive.h"

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
#include <eely/anim_graph/anim_graph_node_sum.h>
#include <eely/anim_graph/anim_graph_uncooked.h>
#include <eely/clip/clip.h>
#include <eely/clip/clip_uncooked.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_uncooked.h>
#include <eely/skeleton_mask/skeleton_mask.h>
#include <eely/skeleton_mask/skeleton_mask_uncooked.h>

#include <entt/entt.hpp>

#include <gsl/util>

#include <imgui.h>

#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0x31363DFF};

static const string_id param_id_speed{"speed"};
static const string_id param_id_look_angle{"look_angle"};
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

  // Skeleton mask
  // This mask is used to remove legs and arms from look animations when computing additive clip,
  // so that we don't add these joints on top of normal movement animations.

  auto& skeleton_mask{project_uncooked.add_resource<skeleton_mask_uncooked>("look_mask")};
  skeleton_mask.set_target_skeleton_id(skeleton_uncooked.get_id());
  std::unordered_map<string_id, joint_weight>& joint_weights{skeleton_mask.get_weights()};
  for (const skeleton_uncooked::joint& j : skeleton_uncooked.get_joints()) {
    if (j.id == "mixamorig:Spine" || j.id == "mixamorig:Spine1" || j.id == "mixamorig:Spine2" ||
        j.id == "mixamorig:Neck" || j.id == "mixamorig:Head" || j.id == "mixamorig:HeadTop_End" ||
        j.id == "mixamorig:LeftEye" || j.id == "mixamorig:RightEye") {
      joint_weights[j.id] = joint_weight{.translation = 1.0F, .rotation = 1.0F, .scale = 1.0F};
    }
    else {
      joint_weights[j.id] = joint_weight{.translation = 0.0F, .rotation = 0.0F, .scale = 0.0F};
    }
  }

  // Clips

  importer{project_uncooked, exec_dir / "res/jog.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/run.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walk.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/look_-45.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/look_45.fbx"}.import_clip(0, skeleton_uncooked);

  // Additive clips

  auto& clip_look_left_additive{
      project_uncooked.add_resource<clip_additive_uncooked>("look_-45_additive")};
  clip_look_left_additive.set_target_skeleton_id(skeleton_uncooked.get_id());
  clip_look_left_additive.set_skeleton_mask_id("look_mask");
  clip_look_left_additive.set_base_clip_id("walk");
  clip_look_left_additive.set_base_clip_range(
      clip_additive_uncooked::range{.to_s = 0.0F, .from_s = 0.0F});
  clip_look_left_additive.set_source_clip_id("look_-45");

  auto& clip_look_right_additive{
      project_uncooked.add_resource<clip_additive_uncooked>("look_45_additive")};
  clip_look_right_additive.set_target_skeleton_id(skeleton_uncooked.get_id());
  clip_look_right_additive.set_skeleton_mask_id("look_mask");
  clip_look_right_additive.set_base_clip_id("walk");
  clip_look_right_additive.set_base_clip_range(
      clip_additive_uncooked::range{.to_s = 0.0F, .from_s = 0.0F});
  clip_look_right_additive.set_source_clip_id("look_45");

  // Animation graph:
  //  - three standing movement animations, blended by speed
  //  - two additive look animations, blended by look angle
  //  - standing animation is combined with additive look to produce final pose
  // Additional node controls playback speed of this whole tree.

  auto& graph{project_uncooked.add_resource<anim_graph_uncooked>("graph")};

  graph.set_skeleton_id(skeleton_uncooked.get_id());

  // Clips

  auto& node_walk{graph.add_node<anim_graph_node_clip>()};
  node_walk.set_clip_id("walk");
  node_walk.set_editor_position(float3{712.0F, -138.0F, 0.0F});

  auto& node_jog{graph.add_node<anim_graph_node_clip>()};
  node_jog.set_clip_id("jog");
  node_jog.set_editor_position(float3{712.0F, -50.0F, 0.0F});

  auto& node_run{graph.add_node<anim_graph_node_clip>()};
  node_run.set_clip_id("run");
  node_run.set_editor_position(float3{712.0F, 29.0F});

  auto& node_look_minus_45{graph.add_node<anim_graph_node_clip>()};
  node_look_minus_45.set_clip_id("look_-45_additive");
  node_look_minus_45.set_editor_position(float3{712.0F, 253.0F, 0.0F});

  auto& node_look_plus_45{graph.add_node<anim_graph_node_clip>()};
  node_look_plus_45.set_clip_id("look_45_additive");
  node_look_plus_45.set_editor_position(float3{712.0F, 333.0F, 0.0F});

  // Params

  auto& node_speed_param{graph.add_node<anim_graph_node_param>()};
  node_speed_param.set_param_id(param_id_speed);
  node_speed_param.set_editor_position(float3{712.0F, -238.0F, 0.0F});

  auto& node_look_angle_param{graph.add_node<anim_graph_node_param>()};
  node_look_angle_param.set_param_id(param_id_look_angle);
  node_look_angle_param.set_editor_position(float3{712.0F, 145.0F, 0.0F});

  auto& node_playback_speed_param{graph.add_node<anim_graph_node_param>()};
  node_playback_speed_param.set_param_id(param_id_playback_speed);
  node_playback_speed_param.set_editor_position(float3{232.0F, -65.0F, 0.0F});

  // Blends

  auto& node_blend_movement{graph.add_node<anim_graph_node_blend>()};
  node_blend_movement.get_pose_nodes() = {
      {.id = node_walk.get_id(), .factor = param_speed_walk},
      {.id = node_jog.get_id(), .factor = param_speed_jog},
      {.id = node_run.get_id(), .factor = param_speed_run},
  };
  node_blend_movement.set_factor_node_id(node_speed_param.get_id());
  node_blend_movement.set_editor_position(float3{466.0F, -123.0F, 0.0F});

  auto& node_blend_look{graph.add_node<anim_graph_node_blend>()};
  node_blend_look.get_pose_nodes() = {
      {.id = node_look_minus_45.get_id(), .factor = -45.0F},
      {.id = node_look_plus_45.get_id(), .factor = 45.0F},
  };
  node_blend_look.set_factor_node_id(node_look_angle_param.get_id());
  node_blend_look.set_editor_position(float3{466.0F, 180.0F});

  // Sum node (movement + look)

  auto& node_sum{graph.add_node<anim_graph_node_sum>()};
  node_sum.set_first_node_id(node_blend_movement.get_id());
  node_sum.set_second_node_id(node_blend_look.get_id());
  node_sum.set_editor_position(float3{226.0F, 38.0F, 0.0F});

  // Playback speed node

  auto& node_playback_speed{graph.add_node<anim_graph_node_speed>()};
  node_playback_speed.set_speed_provider_node(node_playback_speed_param.get_id());
  node_playback_speed.set_child_node(node_sum.get_id());
  node_playback_speed.set_editor_position(float3{});

  graph.set_root_node_id(node_playback_speed.get_id());

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_additive::app_example_additive(const unsigned int width,
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
  _params.get_value<float>(param_id_look_angle) = 0.0F;
  _params.get_value<float>(param_id_playback_speed) = 1.0F;

  // Initialize graph editor for visualization

  _anim_graph_editor =
      std::make_unique<anim_graph_editor>(graph, component_anim_graph.player.get());
}

void app_example_additive::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(350.0F, 0.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "Blend", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    float& speed_value{_params.get_value<float>(param_id_speed)};
    ImGui::SliderFloat("Speed", &speed_value, param_speed_walk, param_speed_run, "%.2f");

    float& look_angle{_params.get_value<float>(param_id_look_angle)};
    ImGui::SliderFloat("Look angle", &look_angle, -45.0F, 45.0F, "%.1f");

    float& playback_speed_value{_params.get_value<float>(param_id_playback_speed)};
    ImGui::SliderFloat("Playback speed", &playback_speed_value, 0.0F, 2.0F, "%.2f");

    ImGui::Separator();

    ImGui::Checkbox("Show graph editor", &_show_graph_editor);

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