#include "eely_app/filesystem_utils.h"

#include <fmt/format.h>

#include <gsl/narrow>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <vector>

#if EELY_PLATFORM_WIN64
#include <windows.h>
#endif

namespace eely {
std::filesystem::path get_executable_dir()
{
  static const std::filesystem::path executable_dir = []() {
#if EELY_PLATFORM_WIN64
    std::array<WCHAR, MAX_PATH> path;
    if (GetModuleFileNameW(NULL, path.data(), MAX_PATH) != 0) {
      return std::filesystem::path{path.data()}.parent_path();
    }

    throw std::runtime_error("Couldn't get executable dir");
#else
    static_assert(false, "Platform is not supported");
#endif
  }();

  return executable_dir;
}

std::vector<std::byte> load_binary(const std::filesystem::path& path_absolute)
{
  std::ifstream file{path_absolute, std::ios::binary};
  if (!file) {
    throw std::runtime_error{fmt::format("Could not open file: {}", path_absolute.string())};
  }

  // Get stream size

  const std::istream::pos_type current_pos{file.tellg()};
  if (current_pos == std::istream::pos_type{-1}) {
    throw std::runtime_error{
        fmt::format("Could not calculate file size: {}", path_absolute.string())};
  }

  file.seekg(0, std::istream::end);
  std::istream::pos_type end_pos{file.tellg()};
  file.seekg(current_pos);
  std::streamoff stream_size{end_pos - current_pos};

  // Load data

  std::vector<std::byte> result;
  result.resize(stream_size);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast), old interface
  file.read(reinterpret_cast<char*>(result.data()), gsl::narrow<std::streamsize>(result.size()));

  return result;
}
}  // namespace eely