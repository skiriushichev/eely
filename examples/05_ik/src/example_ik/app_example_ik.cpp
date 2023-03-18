#include "example_ik/app_example_ik.h"

#include <eely_app/app.h>
#include <eely_app/component_camera.h>
#include <eely_app/component_clip.h>
#include <eely_app/component_skeleton.h>
#include <eely_app/component_transform.h>
#include <eely_app/filesystem_utils.h>
#include <eely_app/scene.h>
#include <eely_app/system_camera.h>
#include <eely_app/system_render.h>
#include <eely_app/system_skeleton.h>

#include <eely_importer/importer.h>

#include <eely/clip/clip.h>
#include <eely/math/float4.h>
#include <eely/math/quaternion.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/presets.h>
#include <eely/skeleton/skeleton.h>
#include <eely/skeleton/skeleton_pose.h>
#include <eely/skeleton/skeleton_uncooked.h>

#include <imgui.h>

#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0x31363DFF};

void set_swing_twist(skeleton_pose& pose,
                     gsl::index joint_index,
                     const float twist_rad,
                     const float swing_y_rad,
                     const float swing_z_rad)
{
  const skeleton& skeleton{pose.get_skeleton()};
  const std::optional<gsl::index> parent_joint_index_opt{
      skeleton.get_joint_parent_index(joint_index)};
  if (!parent_joint_index_opt.has_value()) {
    // Root joints are not constrained.
    return;
  }

  const skeleton::constraint& constraint{pose.get_skeleton().get_constraint(joint_index)};
  if (!constraint.limit_twist_rad.has_value() && !constraint.limit_swing_y_rad.has_value() &&
      !constraint.limit_swing_z_rad.has_value()) {
    return;
  }

  const transform& parent_transform{
      pose.get_transform_object_space(parent_joint_index_opt.value())};
  const transform& child_transform{pose.get_transform_object_space(joint_index)};

  const quaternion parent_constraint_orientation{parent_transform.rotation *
                                                 constraint.parent_constraint_delta};
  quaternion child_constraint_orientation{child_transform.rotation *
                                          constraint.child_constraint_delta};

  quaternion delta{quaternion_from_axis_angle(0.0F, 0.0F, 1.0F, swing_z_rad) *
                   quaternion_from_axis_angle(0.0F, 1.0F, 0.0F, swing_y_rad) *
                   quaternion_from_axis_angle(1.0F, 0.0F, 0.0F, twist_rad)};

  child_constraint_orientation = parent_constraint_orientation * delta;

  const quaternion new_child_orientation{child_constraint_orientation *
                                         quaternion_inverse(constraint.child_constraint_delta)};

  const transform new_child_transform_joint_space{
      transform_inverse(parent_transform) *
      transform{child_transform.translation, new_child_orientation, child_transform.scale}};
  pose.set_transform_joint_space(joint_index, new_child_transform_joint_space);
}

static std::unique_ptr<project> import_and_cook_resources()
{
  const std::filesystem::path fbx_path{get_executable_dir() / "res/sitting_clap.fbx"};

  // Import resources (a skeleton and an animation clip) from FBX into uncooked project
  // and cook into a runtime project.
  // This can also be done offline in the editor.

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};
  importer importer{project_uncooked, fbx_path};

  skeleton_uncooked& skeleton_uncooked{importer.import_skeleton(0)};
  mixamo_init(skeleton_uncooked);

  clip_uncooked& clip_uncooked{importer.import_clip(0, skeleton_uncooked)};
  clip_uncooked.set_compression_scheme(clip_compression_scheme::fixed);

  // Convert into runtime project

  static constexpr size_t buffer_size_bytes{gsl::narrow<size_t>(1024 * 256)};
  std::array<std::byte, buffer_size_bytes> buffer;
  project::cook(project_uncooked, buffer);
  return std::make_unique<project>(buffer);
}

app_example_ik::app_example_ik(const unsigned int width,
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

  // Create characters
  // Additional character is rendered to see the difference between
  // constrained and unconstrained skeletons.

  const skeleton& skeleton{*_project->get_resource<eely::skeleton>("mixamorig:Hips")};

  _character = registry.create();
  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));

  _character_unconstrained = registry.create();
  registry.emplace<component_transform>(_character_unconstrained, transform{});
  auto& s = registry.emplace<component_skeleton>(_character_unconstrained, &skeleton,
                                                 skeleton_pose(skeleton));
  s.pose_render_color = float4{0.8F, 0.8F, 0.80F, 0.2F};

  // Init data for modes

  _constraints_selected_joint_id = skeleton.get_joint_id(0);
}

void app_example_ik::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  _scene.update(dt_s);

  entt::registry& registry{_scene.get_registry()};
  component_skeleton& component_skeleton{registry.get<eely::component_skeleton>(_character)};

  ImGui::SetNextWindowSize(ImVec2(340.0F, 280.0F), ImGuiCond_Once);
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F), ImGuiCond_Once);
  if (ImGui::Begin("IK", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    // Mode selection
    int mode_int{static_cast<int>(_mode)};
    ImGui::TextUnformatted("Mode: ");
    ImGui::SameLine();
    ImGui::RadioButton("IK", &mode_int, static_cast<int>(mode::ik));
    ImGui::SameLine();
    ImGui::RadioButton("Constraints", &mode_int, static_cast<int>(mode::constraints));
    _mode = static_cast<mode>(mode_int);

    ImGui::Separator();

    switch (_mode) {
      case mode::ik: {
        component_skeleton.joint_renders.clear();
      } break;

      case mode::constraints: {
        const skeleton& skeleton{*_project->get_resource<eely::skeleton>("mixamorig:Hips")};

        // Joint selector
        if (ImGui::BeginCombo("Joint", _constraints_selected_joint_id.c_str())) {
          for (gsl::index i{0}; i < skeleton.get_joints_count(); i++) {
            const string_id& id{skeleton.get_joint_id(i)};
            const bool selected{_constraints_selected_joint_id == id};
            if (ImGui::Selectable(id.c_str(), selected)) {
              _constraints_selected_joint_id = id;
            }
          }
          ImGui::EndCombo();
        }

        // Render flags selection

        ImGui::Checkbox("Render frame", &_constraints_render_joint_frame);
        ImGui::Checkbox("Render parent constraint frame",
                        &_constraints_render_parent_constraint_frame);
        ImGui::Checkbox("Render child constraint frame",
                        &_constraints_render_child_constraint_frame);
        ImGui::Checkbox("Render constraint limits", &_constraints_render_limits);

        using flags = component_skeleton::joint_render_flags;

        int joint_renders{flags::none};

        if (_constraints_render_joint_frame) {
          joint_renders |= static_cast<int>(flags::frame);
        }
        if (_constraints_render_parent_constraint_frame) {
          joint_renders |= static_cast<int>(flags::constraint_parent_frame);
        }
        if (_constraints_render_child_constraint_frame) {
          joint_renders |= static_cast<int>(flags::constraint_child_frame);
        }
        if (_constraints_render_limits) {
          joint_renders |= static_cast<int>(flags::constraint_limits);
        }

        component_skeleton.joint_renders = {
            {_constraints_selected_joint_id, static_cast<flags>(joint_renders)}};

        // Swing twist control

        const gsl::index joint_index{
            component_skeleton.skeleton->get_joint_index(_constraints_selected_joint_id).value()};

        // TODO: init twist and swing from the rest pose
        static float twist_deg{0.0F};
        static float swing_y_deg{0.0F};
        static float swing_z_deg{0.0F};

        ImGui::SliderFloat("Twist", &twist_deg, -180.0F, 180.0F, "%.2F");
        ImGui::SliderFloat("Swing Y", &swing_y_deg, -180.0F, 180.0F, "%.2F");
        ImGui::SliderFloat("Swing Z", &swing_z_deg, -180.0F, 180.0F, "%.2F");

        set_swing_twist(component_skeleton.pose, joint_index, deg_to_rad(twist_deg),
                        deg_to_rad(swing_y_deg), deg_to_rad(swing_z_deg));

        eely::component_skeleton& component_skeleton_unconstrained{
            registry.get<eely::component_skeleton>(_character_unconstrained)};
        component_skeleton_unconstrained.pose = component_skeleton.pose;

        constraint_force(component_skeleton.pose);

      } break;
    }

    ImGui::End();
  }
}
}  // namespace eely