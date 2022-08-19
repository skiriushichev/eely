#include "example_btree/app_example_btree.h"

#include <eely_app/app.h>
#include <eely_app/component_camera.h>
#include <eely_app/component_skeleton.h>
#include <eely_app/component_transform.h>
#include <eely_app/filesystem_utils.h>
#include <eely_app/scene.h>
#include <eely_app/system_camera.h>
#include <eely_app/system_render.h>
#include <eely_app/system_skeleton.h>

#include <eely_importer/importer.h>

#include "eely/skeleton/skeleton_uncooked.h"
#include <eely/base/bit_writer.h>
#include <eely/btree/btree_player_node_add.h>
#include <eely/btree/btree_player_node_blend.h>
#include <eely/btree/btree_player_node_clip.h>
#include <eely/clip/clip.h>
#include <eely/clip/clip_uncooked.h>
#include <eely/math/quaternion.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton_mask/skeleton_mask_uncooked.h>

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
  // Create or import resources (a skeleton, animation clips and a skeleton mask) from FBX into
  // uncooked project and cook into a runtime project. This can also be done offline in the editor

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

  static constexpr size_t buffer_size_bytes{gsl::narrow_cast<size_t>(1024 * 1024)};
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

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));

  // Create blend tree player
  // Three standing movement animations, blended by speed
  // Two crouching movement animations, blended by speed
  // Standing and movement animations are blended by crouching parameter
  // On top, additive look is added which is controlled by angle parameter
  // TODO: this should be made by `eely::btree::create_player`, once btree resource is ready

  using namespace eely::internal;

  std::vector<std::unique_ptr<btree_player_node_base>> nodes;

  const clip& clip_walk{*_project->get_resource<eely::clip>("walk")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_walk));

  const clip& clip_jog{*_project->get_resource<eely::clip>("jog")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_jog));

  const clip& clip_run{*_project->get_resource<eely::clip>("run")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_run));

  nodes.push_back(std::make_unique<btree_player_node_blend>(
      param_id_speed, std::vector<btree_player_node_blend::child>{
                          btree_player_node_blend::child{*nodes[0], 1.0F},
                          btree_player_node_blend::child{*nodes[1], 2.0F},
                          btree_player_node_blend::child{*nodes[2], 3.0F}}));

  const clip& clip_walk_crouch{*_project->get_resource<eely::clip>("walk_crouch")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_walk_crouch));

  const clip& clip_run_crouch{*_project->get_resource<eely::clip>("run_crouch")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_run_crouch));

  nodes.push_back(std::make_unique<btree_player_node_blend>(
      param_id_speed, std::vector<btree_player_node_blend::child>{
                          btree_player_node_blend::child{*nodes[4], 1.0F},
                          btree_player_node_blend::child{*nodes[5], 3.0F}}));

  nodes.push_back(std::make_unique<btree_player_node_blend>(
      param_id_crouch, std::vector<btree_player_node_blend::child>{
                           btree_player_node_blend::child{*nodes[3], 0.0F},
                           btree_player_node_blend::child{*nodes[6], 1.0F}}));

  const clip& clip_look_left{*_project->get_resource<eely::clip>("look_left")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_look_left));

  const clip& clip_look_right{*_project->get_resource<eely::clip>("look_right")};
  nodes.push_back(std::make_unique<btree_player_node_clip>(clip_look_right));

  nodes.push_back(std::make_unique<btree_player_node_blend>(
      param_id_look_angle, std::vector<btree_player_node_blend::child>{
                               btree_player_node_blend::child{*nodes[8], -45.0F},
                               btree_player_node_blend::child{*nodes[9], 45.0F}}));

  nodes.push_back(std::make_unique<btree_player_node_add>(*nodes[7], *nodes[10]));

  _btree_player = std::make_unique<btree_player>(skeleton, std::move(nodes), _params);

  _params.set_float(param_id_speed, 1.0F);
  _params.set_float(param_id_crouch, 0.0F);
  _params.set_float(param_id_look_angle, 0.0F);
}

void app_example_btree::update(const float dt_s)
{
  using namespace eely::internal;

  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(250.0F, 100.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "Btree player", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    float param_value_speed{_params.get_float(param_id_speed)};
    if (ImGui::SliderFloat("Speed", &param_value_speed, 1.0F, 3.0F, "%.2f", 1.0F)) {
      _params.set_float(param_id_speed, param_value_speed);
    }

    float param_value_crouch{_params.get_float(param_id_crouch)};
    if (ImGui::SliderFloat("Crouch", &param_value_crouch, 0.0F, 1.0F, "%.2f", 1.0F)) {
      _params.set_float(param_id_crouch, param_value_crouch);
    }

    float param_value_look_angle{_params.get_float(param_id_look_angle)};
    if (ImGui::SliderFloat("Look angle", &param_value_look_angle, -45.0F, 45.0F, "%.2f", 1.0F)) {
      _params.set_float(param_id_look_angle, param_value_look_angle);
    }

    ImGui::End();
  }

  entt::registry& registry{_scene.get_registry()};
  component_skeleton& component_skeleton{registry.get<eely::component_skeleton>(_character)};

  // TODO: this should be done by skeleton system once btree resource is ready
  _btree_player->play(dt_s, component_skeleton.pose);

  _scene.update(dt_s);
}
}  // namespace eely