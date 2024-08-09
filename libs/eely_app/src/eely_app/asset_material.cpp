#include "eely_app/asset_material.h"

#include "eely_app/filesystem_utils.h"

#include <gsl/narrow>

#include <cstddef>
#include <vector>

namespace eely {
bool asset_material::key::operator==(const key& other) const
{
  return path_vertex_shader == other.path_vertex_shader &&
         path_fragment_shader == other.path_fragment_shader;
}

bgfx::ShaderHandle create_shader(const std::filesystem::path& path)
{
  const std::vector<std::byte> binary{load_binary(path)};
  const bgfx::Memory* bgfx_memory{bgfx::copy(binary.data(), gsl::narrow<uint32_t>(binary.size()))};

  return bgfx::createShader(bgfx_memory);
}

asset_material::asset_material(const key& key) : _bgfx_program_handle{bgfx::kInvalidHandle}
{
  bgfx::ShaderHandle bgfx_vshader_handle{create_shader(key.path_vertex_shader)};
  bgfx::ShaderHandle bgfx_fshader_handle{create_shader(key.path_fragment_shader)};

  _bgfx_program_handle = bgfx::createProgram(bgfx_vshader_handle, bgfx_fshader_handle, true);
}

asset_material::~asset_material()
{
  bgfx::destroy(_bgfx_program_handle);
}

bgfx::ProgramHandle asset_material::get_program_handle() const
{
  return _bgfx_program_handle;
}
}  // namespace eely