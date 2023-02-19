#pragma once

#include "eely/base/bit_reader.h"
#include "eely/project/resource_base.h"

#include <unordered_set>

namespace eely {
class project_uncooked;

// Base class for every uncooked resource.
// Uncooked resources cannot be used at runtime, must first be cooked,
// but they have a simpler structure and are more convenient to edit or visualize.
class resource_uncooked : public resource_base {
public:
  explicit resource_uncooked(const project_uncooked& project, internal::bit_reader& reader);
  explicit resource_uncooked(const project_uncooked& project, string_id id);

  // Get project this resource belongs to.
  [[nodiscard]] const project_uncooked& get_project() const;

  // Collect ids of all dependencies for this resource.
  virtual void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const;

private:
  const project_uncooked& _project;
};

namespace internal {
// Serialize uncooked resource's type and data into a memory buffer.
void resource_uncooked_serialize(const resource_uncooked& resource, bit_writer& writer);

// Create and deserialize uncooked resource from a memory buffer.
std::unique_ptr<resource_uncooked> resource_uncooked_deserialize(const project_uncooked& project,
                                                                 bit_reader& reader);
}  // namespace internal
}  // namespace eely