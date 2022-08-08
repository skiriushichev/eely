#include "eely_app/asset_mesh.h"

#include <eely/base/string_id.h>

#include <bgfx/bgfx.h>

#include <unordered_map>

namespace eely {
// This is not a globally accessible variable, nothing wrong here
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unordered_map<string_id, asset_mesh::runtime_mesh_builder> asset_mesh::builders;

void asset_mesh::runtime_mesh_register_builder(const string_id& id, runtime_mesh_builder builder)
{
  builders[id] = std::move(builder);
}

bool asset_mesh::runtime_mesh_is_builder_registred(const string_id& id)
{
  return builders.contains(id);
}

bool asset_mesh::runtime_key::operator==(const asset_mesh::runtime_key& other) const
{
  return id == other.id;
}

asset_mesh::asset_mesh(const runtime_key& key)
{
  Expects(builders.contains(key.id));

  const runtime_mesh_build_result& build_result{builders[key.id](key.id)};
  _bgfx_vbuffer_handle = build_result.bgfx_vbuffer_handle;
  _bgfx_ibuffer_handle = build_result.bgfx_ibuffer_handle;
}

asset_mesh::~asset_mesh()
{
  bgfx::destroy(_bgfx_vbuffer_handle);
  bgfx::destroy(_bgfx_ibuffer_handle);
}

bgfx::VertexBufferHandle asset_mesh::get_vbuffer_handle() const
{
  return _bgfx_vbuffer_handle;
}

bgfx::IndexBufferHandle asset_mesh::get_ibuffer_handle() const
{
  return _bgfx_ibuffer_handle;
}
}  // namespace eely