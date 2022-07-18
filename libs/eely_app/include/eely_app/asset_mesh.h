#pragma once

#include "eely_app/base_utils.h"

#include <eely/string_id.h>

#include <bgfx/bgfx.h>

#include <functional>
#include <unordered_map>

namespace eely {
// Represents mesh asset as a combination of vertex and index buffers.
class asset_mesh final {
public:
  struct runtime_mesh_build_result final {
    bgfx::VertexBufferHandle bgfx_vbuffer_handle;
    bgfx::IndexBufferHandle bgfx_ibuffer_handle;
  };

  using runtime_mesh_builder = std::function<runtime_mesh_build_result(const string_id&)>;

  // Key for runtime meshes.
  // Runtime meshes are created in code by the builders,
  // instead of being loaded from a file.
  // For such meshes builder must be registred using `runtime_mesh_register_builder`.
  struct runtime_key final {
    std::string id;

    bool operator==(const runtime_key& other) const;
  };

  static void runtime_mesh_register_builder(const string_id& id, runtime_mesh_builder builder);
  static bool runtime_mesh_is_builder_registred(const string_id& id);

  // Create material with specified key.
  asset_mesh(const runtime_key& key);

  asset_mesh(const asset_mesh&) = delete;
  asset_mesh(asset_mesh&&) = delete;

  ~asset_mesh();

  asset_mesh& operator=(const asset_mesh&) = delete;
  asset_mesh& operator=(asset_mesh&&) = delete;

  // Get vertex buffer's bgfx handle for rendering.
  [[nodiscard]] bgfx::VertexBufferHandle get_vbuffer_handle() const;

  // Get index buffer's bgfx handle for rendering.
  [[nodiscard]] bgfx::IndexBufferHandle get_ibuffer_handle() const;

private:
  // This is not a globally accessible variable, nothing wrong here
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  static std::unordered_map<string_id, runtime_mesh_builder> builders;

  bgfx::VertexBufferHandle _bgfx_vbuffer_handle;
  bgfx::IndexBufferHandle _bgfx_ibuffer_handle;
};
}  // namespace eely

namespace std {
template <>
struct hash<eely::asset_mesh::runtime_key> {
  size_t operator()(const eely::asset_mesh::runtime_key& key) const
  {
    using namespace eely;

    size_t result{0};
    hash_combine(result, key.id);

    return result;
  }
};
}  // namespace std