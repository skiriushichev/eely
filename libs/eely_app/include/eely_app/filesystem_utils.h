#include <cstddef>
#include <filesystem>
#include <vector>

namespace eely {
// Return path to currently running executable.
std::filesystem::path get_executable_dir();

// Return binary file's content.
std::vector<std::byte> load_binary(const std::filesystem::path& path_absolute);
}  // namespace eely