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

#include <eely/anim_graph/btree/btree.h>
#include <eely/anim_graph/btree/btree_node_add.h>
#include <eely/anim_graph/btree/btree_node_base.h>
#include <eely/anim_graph/btree/btree_node_blend.h>
#include <eely/anim_graph/btree/btree_node_clip.h>
#include <eely/anim_graph/btree/btree_uncooked.h>
#include <eely/anim_graph/fsm/fsm_node_btree.h>
#include <eely/anim_graph/fsm/fsm_node_fsm.h>
#include <eely/anim_graph/fsm/fsm_node_transition.h>
#include <eely/anim_graph/fsm/fsm_uncooked.h>
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

static const string_id param_id_walking{"walking"};

static std::unique_ptr<project> import_and_cook_resources()
{
  // Create or import resources from FBX (a skeleton, animation clips, a skeleton mask and a blend
  // tree) into uncooked project and cook into a runtime project. This can also be done offline in
  // the editor

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};

  const std::filesystem::path exec_dir{get_executable_dir()};

  // Skeleton

  const std::filesystem::path skeleton_fbx_path{exec_dir / "res/idle.fbx"};
  importer skeleton_importer{project_uncooked, skeleton_fbx_path};
  const skeleton_uncooked& skeleton_uncooked{skeleton_importer.import_skeleton(0)};

  // Clips

  importer{project_uncooked, exec_dir / "res/idle.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/start_walking.fbx"}.import_clip(0, skeleton_uncooked);
  importer{project_uncooked, exec_dir / "res/walking.fbx"}.import_clip(0, skeleton_uncooked);

  // Blend trees

  {
    auto btree{std::make_unique<btree_uncooked>("btree_idle")};

    btree->set_skeleton_id(skeleton_uncooked.get_id());

    std::vector<anim_graph_node_uptr>& nodes{btree->get_nodes()};

    auto node{std::make_unique<btree_node_clip>()};
    node->set_clip_id("idle");
    nodes.push_back(std::move(node));

    btree->set_root_node_index(0);

    project_uncooked.set_resource(std::move(btree));
  }

  {
    auto btree{std::make_unique<btree_uncooked>("btree_start_walking")};

    btree->set_skeleton_id(skeleton_uncooked.get_id());

    std::vector<anim_graph_node_uptr>& nodes{btree->get_nodes()};

    auto node{std::make_unique<btree_node_clip>()};
    node->set_clip_id("start_walking");
    nodes.push_back(std::move(node));

    btree->set_root_node_index(0);

    project_uncooked.set_resource(std::move(btree));
  }

  {
    auto btree{std::make_unique<btree_uncooked>("btree_walking")};

    btree->set_skeleton_id(skeleton_uncooked.get_id());

    std::vector<anim_graph_node_uptr>& nodes{btree->get_nodes()};

    auto node{std::make_unique<btree_node_clip>()};
    node->set_clip_id("walking");
    nodes.push_back(std::move(node));

    btree->set_root_node_index(0);

    project_uncooked.set_resource(std::move(btree));
  }

  // Walking state machine

  auto walking_fsm{std::make_unique<fsm_uncooked>("fsm_walking")};
  walking_fsm->set_skeleton_id(skeleton_uncooked.get_id());
  auto& walking_fsm_nodes{walking_fsm->get_nodes()};

  auto walking_fsm_start_walking{std::make_unique<fsm_node_btree>()};
  walking_fsm_start_walking->set_btree_id("btree_start_walking");
  walking_fsm_start_walking->get_children_indices() = {2};
  walking_fsm_nodes.push_back(std::move(walking_fsm_start_walking));

  auto walking_fsm_walking{std::make_unique<fsm_node_btree>()};
  walking_fsm_walking->set_btree_id("btree_walking");
  walking_fsm_nodes.push_back(std::move(walking_fsm_walking));

  auto walking_fsm_start_walking_to_walking{std::make_unique<fsm_node_transition>()};
  walking_fsm_start_walking_to_walking->get_children_indices() = {1};
  walking_fsm_start_walking_to_walking->set_trigger(transition_trigger_source_end{});
  walking_fsm_start_walking_to_walking->set_blending(transition_blending::frozen_fade);
  walking_fsm_start_walking_to_walking->set_duration_s(0.0F);
  walking_fsm_nodes.push_back(std::move(walking_fsm_start_walking_to_walking));

  project_uncooked.set_resource(std::move(walking_fsm));

  // Root state machine (idle + walking)

  auto root_fsm{std::make_unique<fsm_uncooked>("fsm_root")};
  root_fsm->set_skeleton_id(skeleton_uncooked.get_id());
  auto& root_fsm_nodes{root_fsm->get_nodes()};

  auto root_fsm_idle{std::make_unique<fsm_node_btree>()};
  root_fsm_idle->set_btree_id("btree_idle");
  root_fsm_idle->get_children_indices() = {2};
  root_fsm_nodes.push_back(std::move(root_fsm_idle));

  auto root_fsm_walking{std::make_unique<fsm_node_fsm>()};
  root_fsm_walking->set_fsm_id("fsm_walking");
  root_fsm_walking->get_children_indices() = {3};
  root_fsm_nodes.push_back(std::move(root_fsm_walking));

  auto root_fsm_idle_to_walking{std::make_unique<fsm_node_transition>()};
  root_fsm_idle_to_walking->get_children_indices() = {1};
  root_fsm_idle_to_walking->set_trigger(
      transition_trigger_param_value{.param_id = "walking", .value = true});
  root_fsm_idle_to_walking->set_blending(transition_blending::frozen_fade);
  root_fsm_idle_to_walking->set_duration_s(0.22F);
  root_fsm_nodes.push_back(std::move(root_fsm_idle_to_walking));

  auto root_fsm_walking_to_idle{std::make_unique<fsm_node_transition>()};
  root_fsm_walking_to_idle->get_children_indices() = {0};
  root_fsm_walking_to_idle->set_trigger(
      transition_trigger_param_value{.param_id = "walking", .value = false});
  root_fsm_walking_to_idle->set_blending(transition_blending::frozen_fade);
  root_fsm_walking_to_idle->set_duration_s(0.23F);
  root_fsm_nodes.push_back(std::move(root_fsm_walking_to_idle));

  project_uncooked.set_resource(std::move(root_fsm));

  // Cook project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 1024)};
  std::array<std::byte, buffer_size_bytes> buffer;
  bit_writer writer{buffer};

  project::cook(project_uncooked, writer);

  // Load runtime project

  bit_reader reader{buffer};
  return std::make_unique<project>(reader);
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
  const fsm& fsm{*_project->get_resource<eely::fsm>("fsm_root")};

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));
  registry.emplace<component_anim_graph>(_character,
                                         std::make_unique<anim_graph_player>(fsm, _params));

  // Initialize default parameter values

  _params.set(param_id_walking, false);
}

void app_example_state_machine::update(const float dt_s)
{
  using namespace eely::internal;

  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  ImGui::SetNextWindowSize(ImVec2(250.0F, 100.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "State machine player", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    bool walking{_params.get_bool(param_id_walking)};
    if (ImGui::Checkbox("Walking", &walking)) {
      _params.set(param_id_walking, walking);
    }

    ImGui::End();
  }

  _scene.update(dt_s);
}
}  // namespace eely