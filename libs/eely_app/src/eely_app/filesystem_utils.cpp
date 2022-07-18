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

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace eely {
std::filesystem::path get_executable_dir()
{
  static const std::filesystem::path executable_dir = []() {
#if defined(__APPLE__)
    std::array<char, PATH_MAX> path_unresolved;
    uint32_t size{path_unresolved.size()};
    int get_path_result{_NSGetExecutablePath(path_unresolved.data(), &size)};

    if (get_path_result == 0) {
      std::array<char, PATH_MAX> path_resolved;
      if (realpath(path_unresolved.data(), path_resolved.data()) == path_resolved.data()) {
        return std::filesystem::path{path_resolved.data()}.parent_path();
      }
    }

    throw std::runtime_error("Couldn't get executable dir");
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