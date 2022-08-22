#include "eely_app/system_render.h"

#include "eely_app/asset_material.h"
#include "eely_app/asset_mesh.h"
#include "eely_app/asset_uniform.h"
#include "eely_app/component_camera.h"
#include "eely_app/component_skeleton.h"
#include "eely_app/component_transform.h"
#include "eely_app/filesystem_utils.h"
#include "eely_app/matrix4x4.h"

#include "eely/skeleton/skeleton_pose.h"

#include <bgfx/bgfx.h>

#include <entt/entity/registry.hpp>

#include <gsl/narrow>

#include <array>
#include <cstdint>

namespace eely {
/* TODO: use later
static void render_coordinate_system(app& app, const transform& transform)
{
  static const string_id coordinate_system_mesh_id{"coordinate_system"};

  if (!asset_mesh::runtime_mesh_is_builder_registred(coordinate_system_mesh_id)) {
    auto builder = [](const string_id&) {
      struct vertex_position_color final {
        float3 position;
        uint32_t color_abgr{0};
      };

      bgfx::VertexLayout bgfx_vlayout;
      bgfx_vlayout.begin()
          .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
          .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
          .end();

      std::array<vertex_position_color, 6> vertices{
          {{.position = float3{0.0F, 0.0F, 0.0F}, .color_abgr = 0xff'00'00'ff},
           {.position = float3{1.0F, 0.0F, 0.0F}, .color_abgr = 0xff'00'00'ff},
           {.position = float3{0.0F, 0.0F, 0.0F}, .color_abgr = 0xff'00'ff'00},
           {.position = float3{0.0F, 1.0F, 0.0F}, .color_abgr = 0xff'00'ff'00},
           {.position = float3{0.0F, 0.0F, 0.0F}, .color_abgr = 0xff'ff'00'00},
           {.position = float3{0.0F, 0.0F, 1.0F}, .color_abgr = 0xff'ff'00'00}}};

      std::array<uint16_t, 6> indices{0, 1, 2, 3, 4, 5};

      asset_mesh::runtime_mesh_build_result result;

      const bgfx::Memory* bgfx_vmemory{
          bgfx::copy(vertices.data(), sizeof(vertex_position_color) * vertices.size())};
      result.bgfx_vbuffer_handle = bgfx::createVertexBuffer(bgfx_vmemory, bgfx_vlayout);

      const bgfx::Memory* bgfx_imemory{
          bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size())};
      result.bgfx_ibuffer_handle = bgfx::createIndexBuffer(bgfx_imemory);

      return result;
    };

    asset_mesh::runtime_mesh_register_builder(coordinate_system_mesh_id, builder);
  }

  asset_mesh::runtime_key key{coordinate_system_mesh_id};
  const asset_mesh& mesh{app.get_meshes_runtime().get(key)};

  const asset_material::key material_key{
      .path_vertex_shader = get_executable_dir() / "res/color.vs",
      .path_fragment_shader = get_executable_dir() / "res/color.fs"};
  const asset_material& material{app.get_materials().get(material_key)};

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_MSAA |
                 BGFX_STATE_PT_LINES);
  bgfx::setVertexBuffer(0, mesh.get_vbuffer_handle());
  bgfx::setIndexBuffer(mesh.get_ibuffer_handle());

  matrix4x4 transform_matrix{matrix4x4_from_transform(transform)};
  bgfx::setTransform(transform_matrix.data());

  bgfx::submit(0, material.get_program_handle());
}
*/

static void render_skeleton(app& app,
                            const component_skeleton& component_skeleton,
                            const component_transform& component_transform)
{
  const gsl::index joints_count{component_skeleton.skeleton->get_joints_count()};

  if (joints_count == 0) {
    return;
  }

  // Vertex buffer

  bgfx::VertexLayout bgfx_vlayout;
  bgfx_vlayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

  if (bgfx::getAvailTransientVertexBuffer(joints_count, bgfx_vlayout) < joints_count) {
    Expects(false);
    return;
  }

  std::vector<float3> vertices;
  for (gsl::index i{0}; i < joints_count; ++i) {
    const transform& t{component_skeleton.pose.get_transform_object_space(i)};
    vertices.push_back(t.translation);
  }

  bgfx::TransientVertexBuffer bgfx_tvbuffer;
  bgfx::allocTransientVertexBuffer(&bgfx_tvbuffer, vertices.size(), bgfx_vlayout);
  memcpy(bgfx_tvbuffer.data, vertices.data(), sizeof(float3) * vertices.size());

  // Index buffer

  std::vector<uint16_t> indices;
  for (gsl::index i{0}; i < joints_count; ++i) {
    const std::optional<gsl::index> parent_index{
        component_skeleton.skeleton->get_joint_parent_index(i)};
    if (!parent_index.has_value()) {
      continue;
    }

    indices.push_back(i);
    indices.push_back(parent_index.value());
  }

  if (bgfx::getAvailTransientIndexBuffer(indices.size(), false) < indices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientIndexBuffer bgfx_tibuffer;
  bgfx::allocTransientIndexBuffer(&bgfx_tibuffer, indices.size(), false);
  memcpy(bgfx_tibuffer.data, indices.data(), sizeof(uint16_t) * indices.size());

  // Submit

  const asset_material::key material_key{
      .path_vertex_shader = get_executable_dir() / "res/solid.vs",
      .path_fragment_shader = get_executable_dir() / "res/solid.fs"};
  const asset_material& material{app.get_materials().get(material_key)};

  const asset_uniform::key uniform_key{.id = "u_color", .bgfx_type = bgfx::UniformType::Vec4};
  const asset_uniform& uniform{app.get_uniforms().get(uniform_key)};

  static float4 black_color{0.0F, 0.0F, 0.0F, 1.0F};
  bgfx::setUniform(uniform.get_uniform_handle(), &black_color);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_MSAA |
                 BGFX_STATE_PT_LINES);
  bgfx::setVertexBuffer(0, &bgfx_tvbuffer, 0, vertices.size());
  bgfx::setIndexBuffer(&bgfx_tibuffer);

  matrix4x4 transform_matrix{matrix4x4_from_transform(component_transform.transform)};
  bgfx::setTransform(transform_matrix.data());

  bgfx::submit(0, material.get_program_handle());
}

void system_render_update(app& app, entt::registry& registry, const float /*dt_s*/)
{
  // Render skeletons
  auto skeletons_view{registry.view<component_transform, component_skeleton>()};
  for (entt::entity entity : skeletons_view) {
    component_skeleton& comp_skeleton{skeletons_view.get<eely::component_skeleton>(entity)};
    component_transform& comp_transform{skeletons_view.get<eely::component_transform>(entity)};
    render_skeleton(app, comp_skeleton, comp_transform);
  }

  // Set view and clip transforms
  auto view{registry.view<component_transform, component_camera>()};
  for (entt::entity entity : view) {
    component_transform& comp_transform{view.get<eely::component_transform>(entity)};
    const matrix4x4 view_transform{
        matrix4x4_inverse(matrix4x4_from_transform(comp_transform.transform))};

    component_camera& comp_camera{view.get<eely::component_camera>(entity)};
    const clip_space_params clip_space_params{
        .fov_x = comp_camera.fov_x,
        .aspect_ratio_x_to_y =
            gsl::narrow<float>(app.get_width()) / gsl::narrow<float>(app.get_height()),
        .near = comp_camera.near,
        .far = comp_camera.far,
        .depth_range = bgfx::getCaps()->homogeneousDepth
                           ? clip_space_depth_range::minus_one_to_plus_one
                           : clip_space_depth_range::zero_to_plus_one};
    const matrix4x4 clip_transform{matrix4x4_clip_space(clip_space_params)};

    bgfx::setViewTransform(0, view_transform.data(), clip_transform.data());
  }
}
}  // namespace eely