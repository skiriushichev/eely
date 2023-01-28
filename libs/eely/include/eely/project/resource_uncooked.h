#pragma once

#include "eely/base/bit_reader.h"
#include "eely/project/resource_base.h"

#include <unordered_set>

namespace eely {
// Base class for every uncooked resource.
// Uncooked resources cannot be used at runtime, must first be cooked,
// but they have a simpler structure and are more convenient to edit or visualize.
class resource_uncooked : public resource_base {
  using resource_base::resource_base;

public:
  // Collect ids of all dependencies for this resource.
  virtual void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const;
};

namespace internal {
// Serialize uncooked resource's type and data into a memory buffer.
void resource_uncooked_serialize(const resource_uncooked& resource, bit_writer& writer);

// Create and deserialize uncooked resource from a memory buffer.
std::unique_ptr<resource_uncooked> resource_uncooked_deserialize(bit_reader& reader);
}  // namespace internal
}  // namespace eely