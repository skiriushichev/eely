#pragma once

#include "eely_app/base_utils.h"

#include <eely/string_id.h>

#include <bgfx/bgfx.h>

namespace eely {
// Represents uniform used by shaders.
class asset_uniform final {
public:
  // Key for identifying and constructing iniforms.
  struct key final {
    const string_id id;
    bgfx::UniformType::Enum bgfx_type{bgfx::UniformType::Enum::End};

    bool operator==(const key& other) const;
  };

  // Create uniform with specified key.
  asset_uniform(const key& key);

  asset_uniform(const asset_uniform&) = delete;
  asset_uniform(asset_uniform&&) = delete;

  ~asset_uniform();

  asset_uniform& operator=(const asset_uniform&) = delete;
  asset_uniform& operator=(asset_uniform&&) = delete;

  // Return uniform's handles used for setting its value.
  [[nodiscard]] bgfx::UniformHandle get_uniform_handle() const;

private:
  bgfx::UniformHandle _bgfx_handle;
};
}  // namespace eely

namespace std {
template <>
struct hash<eely::asset_uniform::key> {
  size_t operator()(const eely::asset_uniform::key& key) const
  {
    using namespace eely;

    size_t result{0};
    hash_combine(result, key.id);
    hash_combine(result, key.bgfx_type);

    return result;
  }
};
}  // namespace std