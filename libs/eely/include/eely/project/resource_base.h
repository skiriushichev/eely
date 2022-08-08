#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <vector>

namespace eely {
// Base class for every resource used within a project.
class resource_base {
public:
  // Construct resource from a memory buffer.
  explicit resource_base(bit_reader& reader);

  // Construct empty resource with specified id.
  explicit resource_base(string_id id);

  resource_base(const resource_base&) = delete;
  resource_base(resource_base&&) = delete;

  virtual ~resource_base() = 0;

  resource_base& operator=(const resource_base&) = delete;
  resource_base& operator=(resource_base&&) = delete;

  // Serialize resource into a memory buffer;
  virtual void serialize(bit_writer& writer) const;

  // Get resource id.
  [[nodiscard]] const string_id& get_id() const;

private:
  const string_id _id;
};
}  // namespace eely