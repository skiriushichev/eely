#include "example_clip/app_example_clip.h"

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

#include <eely/base/bit_writer.h>
#include <eely/clip/clip.h>
#include <eely/math/quaternion.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton.h>

#include <imgui.h>

#include <filesystem>
#include <memory>

namespace eely {
constexpr bgfx::ViewId view_id{0};
constexpr uint32_t view_clear_color{0xDCDCDCFF};

static std::unique_ptr<project> import_and_cook_resources()
{
  const std::filesystem::path fbx_path{get_executable_dir() / "res/sitting_clap.fbx"};

  // Import resources (a skeleton and an animation clip) from FBX into uncooked project
  // and cook into a runtime project.
  // This can also be done offline in the editor

  project_uncooked project_uncooked{measurement_unit::meters, axis_system::y_up_x_right_z_forward};
  importer importer{project_uncooked, fbx_path};

  const skeleton_uncooked& skeleton_uncooked{importer.import_skeleton(0)};
  clip_uncooked& clip_uncooked{importer.import_clip(0, skeleton_uncooked)};
  clip_uncooked.set_compression_scheme(clip_compression_scheme::fixed);

  static constexpr size_t buffer_size_bytes{gsl::narrow_cast<size_t>(1024 * 256)};
  std::array<std::byte, buffer_size_bytes> buffer;
  bit_writer writer{buffer};

  project::cook(project_uncooked, writer);

  // Load runtime project

  bit_reader reader{buffer};
  return std::make_unique<project>(reader);
}

app_example_clip::app_example_clip(const unsigned int width,
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
  const clip& clip{*_project->get_resource<eely::clip>("sitting_clap")};

  _character = registry.create();

  registry.emplace<component_transform>(_character, transform{});
  registry.emplace<component_skeleton>(_character, &skeleton, skeleton_pose(skeleton));
  registry.emplace<component_clip>(_character, clip.create_player());
}

void app_example_clip::update(const float dt_s)
{
  bgfx::setViewRect(view_id, 0, 0, get_width(), get_height());
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, view_clear_color);

  _scene.update(dt_s);

  ImGui::SetNextWindowSize(ImVec2(200.0F, 80.0F));
  ImGui::SetNextWindowPos(ImVec2(10.0F, 10.0F));
  if (ImGui::Begin(
          "Clip player", nullptr,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
    entt::registry& registry{_scene.get_registry()};
    component_clip& component_clip{registry.get<eely::component_clip>(_character)};

    ImGui::SliderFloat("Time", &component_clip.play_time_s, 0.0F,
                       component_clip.player->get_duration_s(), "%.2fs", 1.0F);
    ImGui::SliderFloat("Speed", &component_clip.speed, 0.0F, 3.0F, "%.2f", 1.0F);

    ImGui::End();
  }
}
}  // namespace eely