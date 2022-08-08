#pragma once

#include "eely/project/resource_base.h"

namespace eely {
class project;

// Base class for every cooked resource.
// Cooked resources are used at runtime,
// and their structures and interface are optimized for that purpose.
class resource : public resource_base {
  using resource_base::resource_base;
};

// Serialize resource's type and data into a memory buffer.
void resource_serialize(const resource& resource, bit_writer& writer);

// Create and deserialize resource from a memory buffer.
std::unique_ptr<resource> resource_deserialize(const project& project, bit_reader& reader);
}  // namespace eely