#pragma once

#include "eely_app/base_utils.h"

#include <bgfx/bgfx.h>

#include <filesystem>

namespace eely {
// Represents material used for rendering.
class asset_material final {
public:
  // Key for identifying and constructing materials.
  struct key final {
    std::filesystem::path path_vertex_shader;
    std::filesystem::path path_fragment_shader;

    bool operator==(const key& other) const;
  };

  // Create material with specified key.
  asset_material(const key& key);

  asset_material(const asset_material&) = delete;
  asset_material(asset_material&&) = delete;

  ~asset_material();

  asset_material& operator=(const asset_material&) = delete;
  asset_material& operator=(asset_material&&) = delete;

  // Get material's bgfx program handle for rendering.
  [[nodiscard]] bgfx::ProgramHandle get_program_handle() const;

private:
  bgfx::ProgramHandle _bgfx_program_handle;
};
}  // namespace eely

namespace std {
template <>
struct hash<eely::asset_material::key> {
  size_t operator()(const eely::asset_material::key& key) const
  {
    using namespace eely;

    size_t result{0};
    hash_combine(result, key.path_vertex_shader);
    hash_combine(result, key.path_fragment_shader);

    return result;
  }
};
}  // namespace std