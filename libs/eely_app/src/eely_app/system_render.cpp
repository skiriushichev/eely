#include "eely_app/system_render.h"

#include "eely_app/asset_material.h"
#include "eely_app/asset_mesh.h"
#include "eely_app/asset_uniform.h"
#include "eely_app/component_camera.h"
#include "eely_app/component_skeleton.h"
#include "eely_app/component_transform.h"
#include "eely_app/filesystem_utils.h"
#include "eely_app/matrix4x4.h"

#include <eely/base/base_utils.h>
#include <eely/math/ellipse.h>
#include <eely/math/elliptical_cone.h>
#include <eely/math/float3.h>
#include <eely/math/float4.h>
#include <eely/math/math_utils.h>
#include <eely/math/quaternion.h>
#include <eely/math/transform.h>
#include <eely/skeleton/skeleton_pose.h>

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>

#include <entt/entity/registry.hpp>

#include <gsl/narrow>

#include <array>
#include <cmath>
#include <cstdint>

namespace eely {
static void render_coordinate_system(app& app, const transform& transform)
{
  using namespace eely::internal;

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
          bgfx::copy(vertices.data(), sizeof(vertex_position_color) * size_u32(vertices))};
      result.bgfx_vbuffer_handle = bgfx::createVertexBuffer(bgfx_vmemory, bgfx_vlayout);

      const bgfx::Memory* bgfx_imemory{
          bgfx::copy(indices.data(), sizeof(uint16_t) * size_u32(indices))};
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

[[maybe_unused]] static void render_line(app& app,
                                         const float3& from,
                                         const float3& to,
                                         const transform& parent_transform,
                                         const float4& color)
{
  using namespace eely::internal;

  std::array<uint16_t, 2> indices{0, 1};
  std::array<float3, 2> vertices{from, to};

  bgfx::VertexLayout bgfx_vlayout;
  bgfx_vlayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

  if (bgfx::getAvailTransientVertexBuffer(size_u32(vertices), bgfx_vlayout) < vertices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientVertexBuffer bgfx_tvbuffer;
  bgfx::allocTransientVertexBuffer(&bgfx_tvbuffer, size_u32(vertices), bgfx_vlayout);
  memcpy(bgfx_tvbuffer.data, vertices.data(), sizeof(float3) * vertices.size());

  if (bgfx::getAvailTransientIndexBuffer(size_u32(indices), false) < indices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientIndexBuffer bgfx_tibuffer;
  bgfx::allocTransientIndexBuffer(&bgfx_tibuffer, size_u32(indices), false);
  memcpy(bgfx_tibuffer.data, indices.data(), sizeof(uint16_t) * indices.size());

  // Submit

  const asset_material::key material_key{
      .path_vertex_shader = get_executable_dir() / "res/solid.vs",
      .path_fragment_shader = get_executable_dir() / "res/solid.fs"};
  const asset_material& material{app.get_materials().get(material_key)};

  const asset_uniform::key uniform_key{.id = "u_color", .bgfx_type = bgfx::UniformType::Vec4};
  const asset_uniform& uniform{app.get_uniforms().get(uniform_key)};

  bgfx::setUniform(uniform.get_uniform_handle(), &color);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_MSAA |
                 BGFX_STATE_PT_LINES);
  bgfx::setVertexBuffer(0, &bgfx_tvbuffer, 0, size_u32(vertices));
  bgfx::setIndexBuffer(&bgfx_tibuffer);

  matrix4x4 mat{matrix4x4_from_transform(parent_transform)};
  bgfx::setTransform(mat.data());

  bgfx::submit(0, material.get_program_handle());
}

static void render_constraint(app& app,
                              const transform& parent_constraint_frame,
                              const skeleton::constraint& constraint)
{
  using namespace eely::internal;

  if (!constraint.limit_swing_y_rad.has_value() || !constraint.limit_swing_z_rad.has_value()) {
    // TODO: render twist & separate swing limits
    return;
  }

  // Generate elliptical cone vertices

  std::vector<float3> vertices;

  elliptical_cone cone{elliptical_cone_from_height_and_angles(
      0.1F, constraint.limit_swing_y_rad.value(), constraint.limit_swing_z_rad.value())};

  // Apex
  vertices.push_back(float3::zeroes);

  // Ellipse
  for (float v{0.0F}; v < 2.0F * pi; v += (pi / 60.0F)) {
    float2 vertex{ellipse_point_from_angle(cone.ellipse, v)};
    float z = vertex.x;
    float y = vertex.y;

    vertices.push_back(float3{cone.height, y, z});
  }

  bgfx::VertexLayout bgfx_vlayout;
  bgfx_vlayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

  if (bgfx::getAvailTransientVertexBuffer(size_u32(vertices), bgfx_vlayout) < vertices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientVertexBuffer bgfx_tvbuffer;
  bgfx::allocTransientVertexBuffer(&bgfx_tvbuffer, size_u32(vertices), bgfx_vlayout);
  memcpy(bgfx_tvbuffer.data, vertices.data(), sizeof(float3) * vertices.size());

  // Index buffer

  std::vector<uint16_t> indices;
  for (gsl::index i{1}; i < std::ssize(vertices) - 1; ++i) {
    indices.push_back(0);
    indices.push_back(gsl::narrow<uint16_t>(i));
    indices.push_back(gsl::narrow<uint16_t>(i + 1));
  }

  indices.push_back(0);
  indices.push_back(gsl::narrow<uint16_t>(vertices.size() - 1));
  indices.push_back(1);

  if (bgfx::getAvailTransientIndexBuffer(size_u32(indices), false) < indices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientIndexBuffer bgfx_tibuffer;
  bgfx::allocTransientIndexBuffer(&bgfx_tibuffer, size_u32(indices), false);
  memcpy(bgfx_tibuffer.data, indices.data(), sizeof(uint16_t) * indices.size());

  // Submit

  const asset_material::key material_key{
      .path_vertex_shader = get_executable_dir() / "res/solid.vs",
      .path_fragment_shader = get_executable_dir() / "res/solid.fs"};
  const asset_material& material{app.get_materials().get(material_key)};

  const asset_uniform::key uniform_key{.id = "u_color", .bgfx_type = bgfx::UniformType::Vec4};
  const asset_uniform& uniform{app.get_uniforms().get(uniform_key)};

  static float4 cone_color{0.8F, 0.9F, 1.0F, 0.3F};
  bgfx::setUniform(uniform.get_uniform_handle(), &cone_color);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_MSAA |
                 BGFX_STATE_BLEND_ALPHA);
  bgfx::setVertexBuffer(0, &bgfx_tvbuffer, 0, size_u32(vertices));
  bgfx::setIndexBuffer(&bgfx_tibuffer);

  transform parent_constraint_frame_no_scale{parent_constraint_frame};
  parent_constraint_frame_no_scale.scale = float3::ones;

  matrix4x4 transform_matrix{matrix4x4_from_transform(parent_constraint_frame_no_scale)};
  bgfx::setTransform(transform_matrix.data());

  bgfx::submit(0, material.get_program_handle());
}

static void render_skeleton_pose(app& app,
                                 component_skeleton& component_skeleton,
                                 const component_transform& component_transform)
{
  using namespace eely::internal;

  const gsl::index joints_count{component_skeleton.skeleton->get_joints_count()};

  if (joints_count == 0) {
    return;
  }

  // Vertex buffer

  bgfx::VertexLayout bgfx_vlayout;
  bgfx_vlayout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

  if (bgfx::getAvailTransientVertexBuffer(gsl::narrow<uint32_t>(joints_count), bgfx_vlayout) <
      joints_count) {
    Expects(false);
    return;
  }

  std::vector<float3> vertices;
  for (gsl::index i{0}; i < joints_count; ++i) {
    const transform& t{component_skeleton.pose.get_transform_object_space(i)};
    vertices.push_back(t.translation);
  }

  bgfx::TransientVertexBuffer bgfx_tvbuffer;
  bgfx::allocTransientVertexBuffer(&bgfx_tvbuffer, size_u32(vertices), bgfx_vlayout);
  memcpy(bgfx_tvbuffer.data, vertices.data(), sizeof(float3) * vertices.size());

  // Index buffer

  std::vector<uint16_t> indices;
  for (gsl::index i{0}; i < joints_count; ++i) {
    const std::optional<gsl::index> parent_index{
        component_skeleton.skeleton->get_joint_parent_index(i)};
    if (!parent_index.has_value()) {
      continue;
    }

    indices.push_back(gsl::narrow<uint16_t>(i));
    indices.push_back(gsl::narrow<uint16_t>(parent_index.value()));
  }

  if (bgfx::getAvailTransientIndexBuffer(size_u32(indices), false) < indices.size()) {
    Expects(false);
    return;
  }

  bgfx::TransientIndexBuffer bgfx_tibuffer;
  bgfx::allocTransientIndexBuffer(&bgfx_tibuffer, size_u32(indices), false);
  memcpy(bgfx_tibuffer.data, indices.data(), sizeof(uint16_t) * indices.size());

  // Submit

  const asset_material::key material_key{
      .path_vertex_shader = get_executable_dir() / "res/solid.vs",
      .path_fragment_shader = get_executable_dir() / "res/solid.fs"};
  const asset_material& material{app.get_materials().get(material_key)};

  const asset_uniform::key uniform_key{.id = "u_color", .bgfx_type = bgfx::UniformType::Vec4};
  const asset_uniform& uniform{app.get_uniforms().get(uniform_key)};

  bgfx::setUniform(uniform.get_uniform_handle(), &component_skeleton.pose_render_color);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_MSAA |
                 BGFX_STATE_PT_LINES |
                 BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
  bgfx::setVertexBuffer(0, &bgfx_tvbuffer, 0, size_u32(vertices));
  bgfx::setIndexBuffer(&bgfx_tibuffer);

  matrix4x4 transform_matrix{matrix4x4_from_transform(component_transform.transform)};
  bgfx::setTransform(transform_matrix.data());

  bgfx::submit(0, material.get_program_handle());
}

static void render_skeleton_joints(app& app,
                                   component_skeleton& component_skeleton,
                                   const component_transform& component_transform)
{
  using namespace eely::internal;

  for (const auto& [id, flags] : component_skeleton.joint_renders) {
    if (flags == component_skeleton::joint_render_flags::none) {
      continue;
    }

    const std::optional<gsl::index> index_opt{component_skeleton.skeleton->get_joint_index(id)};
    if (!index_opt.has_value()) {
      continue;
    }

    const gsl::index index{index_opt.value()};

    const transform joint_world_transform{
        component_transform.transform * component_skeleton.pose.get_transform_object_space(index)};

    if (has_flag(flags, component_skeleton::joint_render_flags::frame)) {
      transform frame_transform{joint_world_transform};
      frame_transform.scale = float3{0.3F, 0.3F, 0.3F};
      render_coordinate_system(app, frame_transform);
    }

    const std::optional<gsl::index> parent_index_opt{
        component_skeleton.skeleton->get_joint_parent_index(index)};

    const skeleton::constraint& constraint{component_skeleton.skeleton->get_constraint(index)};

    if (has_flag(flags, component_skeleton::joint_render_flags::constraint_parent_frame)) {
      if (parent_index_opt.has_value()) {
        const gsl::index parent_index{parent_index_opt.value()};

        const transform parent_joint_world_transform{
            component_transform.transform *
            component_skeleton.pose.get_transform_object_space(parent_index)};

        transform constraint_frame{component_transform.transform * parent_joint_world_transform};
        constraint_frame.translation = joint_world_transform.translation;
        constraint_frame.rotation = constraint_frame.rotation * constraint.parent_constraint_delta;
        constraint_frame.scale = float3{0.5F, 0.5F, 0.5F};

        render_coordinate_system(app, constraint_frame);
      }
    }

    if (has_flag(flags, component_skeleton::joint_render_flags::constraint_child_frame)) {
      transform constraint_frame{component_transform.transform * joint_world_transform};
      constraint_frame.rotation = constraint_frame.rotation * constraint.child_constraint_delta;
      constraint_frame.scale = float3{0.5F, 0.5F, 0.5F};

      render_coordinate_system(app, constraint_frame);
    }

    if (has_flag(flags, component_skeleton::joint_render_flags::constraint_limits)) {
      if (parent_index_opt.has_value()) {
        const gsl::index parent_index{parent_index_opt.value()};

        const transform parent_joint_transform{
            component_transform.transform *
            component_skeleton.pose.get_transform_object_space(parent_index)};

        transform constraint_frame{component_transform.transform * parent_joint_transform};
        constraint_frame.translation = joint_world_transform.translation;
        constraint_frame.rotation = constraint_frame.rotation * constraint.parent_constraint_delta;
        constraint_frame.scale = float3{0.5F, 0.5F, 0.5F};

        render_constraint(app, constraint_frame,
                          component_skeleton.skeleton->get_constraint(index));
      }
    }
  }
}

void system_render_update(app& app, entt::registry& registry, const float /*dt_s*/)
{
  // Render skeletons
  auto skeletons_view{registry.view<component_transform, component_skeleton>()};
  for (entt::entity entity : skeletons_view) {
    component_skeleton& comp_skeleton{skeletons_view.get<eely::component_skeleton>(entity)};
    component_transform& comp_transform{skeletons_view.get<eely::component_transform>(entity)};
    render_skeleton_pose(app, comp_skeleton, comp_transform);
    render_skeleton_joints(app, comp_skeleton, comp_transform);
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