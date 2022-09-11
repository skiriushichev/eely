#include "example_btree/app_example_btree.h"

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

#include <eely/anim_graph/btree/btree.h>
#include <eely/anim_graph/btree/btree_node_add.h>
#include <eely/anim_graph/btree/btree_node_base.h>
#include <eely/anim_graph/btree/btree_node_blend.h>
#include <eely/anim_graph/btree/btree_node_clip.h>
#include <eely/anim_graph/btree/btree_uncooked.h>
#include <eely/base/bit_writer.h>
#include <eely/clip/clip.h>
#include <eely/clip/clip_uncooked.h>
#include <eely/math/quaternion.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_uncooked.h>
#include <eely/skeleton_mask/skeleton_mask_uncooked.h>

#include <gsl/util>

#include <imgui.h>

#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0xDCDCDCFF};

static const string_id param_id_speed{"speed"};
static const string_id param_id_crouch{"crouch"};
static const string_id param_id_look_angle{"look_angle"};

static std::unique_ptr<project> import_and_cook_resources()
{
  // Create or import resources from FBX (a skeleton, animation clips, a skeleton mask and a blend
  // tree) into uncooked project and cook into a runtime project. This can also be done offline in
  // the editor

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};

  const std::filesystem::path exec_dir{get_executable_dir()};

  // Skeleton

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/walk.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/jog.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/look_-45.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/look_45.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/run_crouch.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/run.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walk_crouch.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walk.fbx"}.import_clip(0, skeleton_uncooked);

  // Skeleton mask
  // This mask is used to remove legs and arms from look animations when computing additive clip,
  // so that we don't add these joints on top of normal movement animations

  auto skeleton_mask{std::make_unique<skeleton_mask_uncooked>("look_mask")};
  skeleton_mask->set_target_skeleton_id(skeleton_uncooked.get_id());
  std::unordered_map<string_id, float>& joint_weights{skeleton_mask->get_weights()};
  for (const skeleton_uncooked::joint& j : skeleton_uncooked.get_joints()) {
    if (j.id == "mixamorig:Spine" || j.id == "mixamorig:Spine1" || j.id == "mixamorig:Spine2" ||
        j.id == "mixamorig:Neck" || j.id == "mixamorig:Head" || j.id == "mixamorig:HeadTop_End" ||
        j.id == "mixamorig:LeftEye" || j.id == "mixamorig:RightEye") {
      joint_weights[j.id] = 1.0F;
    }
    else {
      joint_weights[j.id] = 0.0F;
    }
  }
  project_uncooked.set_resource(std::move(skeleton_mask));

  // Additive clips

  auto clip_look_left_additive{std::make_unique<clip_additive_uncooked>("look_left")};
  clip_look_left_additive->set_target_skeleton_id(skeleton_uncooked.get_id());
  clip_look_left_additive->set_skeleton_mask_id("look_mask");
  clip_look_left_additive->set_base_clip_id("walk");
  clip_look_left_additive->set_base_clip_range(
      clip_additive_uncooked::range{.to_s = 0.0F, .from_s = 0.0F});
  clip_look_left_additive->set_source_clip_id("look_-45");
  project_uncooked.set_resource(std::move(clip_look_left_additive));

  auto clip_look_right_additive{std::make_unique<clip_additive_uncooked>("look_right")};
  clip_look_right_additive->set_target_skeleton_id(skeleton_uncooked.get_id());
  clip_look_right_additive->set_skeleton_mask_id("look_mask");
  clip_look_right_additive->set_base_clip_id("walk");
  clip_look_right_additive->set_base_clip_range(
      clip_additive_uncooked::range{.to_s = 0.0F, .from_s = 0.0F});
  clip_look_right_additive->set_source_clip_id("look_45");
  project_uncooked.set_resource(std::move(clip_look_right_additive));

  // Blend tree
  // Three standing movement animations, blended by speed
  // Two crouching movement animations, blended by speed
  // Standing and movement animations are blended by crouching parameter
  // On top, additive look is added which is controlled by angle parameter

  auto btree{std::make_unique<btree_uncooked>("btree")};

  btree->set_skeleton_id(skeleton_uncooked.get_id());

  std::vector<anim_graph_node_uptr>& nodes{btree->get_nodes()};

  auto node_walk{std::make_unique<btree_node_clip>()};
  node_walk->set_clip_id("walk");
  nodes.push_back(std::move(node_walk));

  auto node_jog{std::make_unique<btree_node_clip>()};
  node_jog->set_clip_id("jog");
  nodes.push_back(std::move(node_jog));

  auto node_run{std::make_unique<btree_node_clip>()};
  node_run->set_clip_id("run");
  nodes.push_back(std::move(node_run));

  auto node_walk_crouch{std::make_unique<btree_node_clip>()};
  node_walk_crouch->set_clip_id("walk_crouch");
  nodes.push_back(std::move(node_walk_crouch));

  auto node_run_crouch{std::make_unique<btree_node_clip>()};
  node_run_crouch->set_clip_id("run_crouch");
  nodes.push_back(std::move(node_run_crouch));

  auto node_look_left{std::make_unique<btree_node_clip>()};
  node_look_left->set_clip_id("look_left");
  nodes.push_back(std::move(node_look_left));

  auto node_look_right{std::make_unique<btree_node_clip>()};
  node_look_right->set_clip_id("look_right");
  nodes.push_back(std::move(node_look_right));

  auto node_blend_walk_jog_run{std::make_unique<btree_node_blend>()};
  node_blend_walk_jog_run->set_param_id("speed");
  node_blend_walk_jog_run->get_children_indices() = {0, 1, 2};
  node_blend_walk_jog_run->get_children_param_values() = {1.0F, 2.0F, 3.0F};
  nodes.push_back(std::move(node_blend_walk_jog_run));

  auto node_blend_walk_run_crouch{std::make_unique<btree_node_blend>()};
  node_blend_walk_run_crouch->set_param_id("speed");
  node_blend_walk_run_crouch->get_children_indices() = {3, 4};
  node_blend_walk_run_crouch->get_children_param_values() = {1.0F, 3.0F};
  nodes.push_back(std::move(node_blend_walk_run_crouch));

  auto node_blend_stand_crouch{std::make_unique<btree_node_blend>()};
  node_blend_stand_crouch->set_param_id("crouch");
  node_blend_stand_crouch->get_children_indices() = {7, 8};
  node_blend_stand_crouch->get_children_param_values() = {0.0F, 1.0F};
  nodes.push_back(std::move(node_blend_stand_crouch));

  auto node_blend_look{std::make_unique<btree_node_blend>()};
  node_blend_look->set_param_id("look_angle");
  node_blend_look->get_children_indices() = {5, 6};
  node_blend_look->get_children_param_values() = {-45.0F, 45.0F};
  nodes.push_back(std::move(node_blend_look));

  auto node_add_movement_and_look{std::make_unique<btree_node_add>()};
  node_add_movement_and_look->get_children_indices() = {9, 10};
  nodes.push_back(std::move(node_add_movement_and_look));

  btree->set_root_node_index(11);

  project_uncooked.set_resource(std::move(btree));

  // Cook project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  bit_writer writer{buffer};

  project::cook(project_uncooked, writer);

  // Load runtime project

  bit_reader reader{buffer};
  return std::make_unique<project>(reader);
}

app_example_btree::app_example_btree(const unsigned int width,
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
  const btree& btree{*_project->get_resource<eely::btree>("btree")};

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));
  registry.emplace<component_anim_graph>(_character,
                                         std::make_unique<anim_graph_player>(btree, _params));

  // Initialize default parameter values

  _params.set(param_id_speed, 1.0F);
  _params.set(param_id_crouch, 0.0F);
  _params.set(param_id_look_angle, 0.0F);
}

void app_example_btree::update(const float dt_s)
{
  using namespace eely::internal;

  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(250.0F, 100.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "Blend tree player", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    float param_value_speed{_params.get_float(param_id_speed)};
    if (ImGui::SliderFloat("Speed", &param_value_speed, 1.0F, 3.0F, "%.2f", 1.0F)) {
      _params.set(param_id_speed, param_value_speed);
    }

    float param_value_crouch{_params.get_float(param_id_crouch)};
    if (ImGui::SliderFloat("Crouch", &param_value_crouch, 0.0F, 1.0F, "%.2f", 1.0F)) {
      _params.set(param_id_crouch, param_value_crouch);
    }

    float param_value_look_angle{_params.get_float(param_id_look_angle)};
    if (ImGui::SliderFloat("Look angle", &param_value_look_angle, -45.0F, 45.0F, "%.2f", 1.0F)) {
      _params.set(param_id_look_angle, param_value_look_angle);
    }

    ImGui::End();
  }

  _scene.update(dt_s);
}
}  // namespace eely