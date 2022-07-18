#include "eely_app/asset_uniform.h"

#include <bgfx/bgfx.h>

namespace eely {
bool asset_uniform::key::operator==(const key& other) const
{
  return id == other.id && bgfx_type == other.bgfx_type;
}

asset_uniform::asset_uniform(const key& key)
    : _bgfx_handle{bgfx::createUniform(key.id.c_str(), key.bgfx_type)}
{
}

asset_uniform::~asset_uniform()
{
  bgfx::destroy(_bgfx_handle);
}

bgfx::UniformHandle asset_uniform::get_uniform_handle() const
{
  return _bgfx_handle;
}
}  // namespace eely