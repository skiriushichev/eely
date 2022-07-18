#pragma once

#include "eely/bit_reader.h"
#include "eely/resource_base.h"

namespace eely {
// Base class for every uncooked resource.
// Uncooked resources cannot be used at runtime, must first be cooked,
// but they have a simpler structure and are more convenient to edit or visualize.
class resource_uncooked : public resource_base {
  using resource_base::resource_base;

public:
  // Collect ids of all dependencies for this resource.
  virtual void collect_dependencies(std::vector<string_id>& out_dependencies) const;
};

// Serialize uncooked resource's type and data into a memory buffer.
void resource_uncooked_serialize(const resource_uncooked& resource, bit_writer& writer);

// Create and deserialize uncooked resource from a memory buffer.
std::unique_ptr<resource_uncooked> resource_uncooked_deserialize(bit_reader& reader);
}  // namespace eely