#pragma once

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource.h"

#include <memory>
#include <unordered_map>

namespace eely {
// Represents a set of cooked resources used in an application.
class project final {
public:
  // Create project from a memory buffer.
  explicit project(bit_reader& reader);

  ~project() = default;

  project(const project&) = delete;
  project(project&&) = delete;

  project& operator=(const project&) = delete;
  project& operator=(project&&) = delete;

  // Get resource with specified id and type.
  // Return `nullptr` if there is no such resource.
  template <typename TRes>
  requires std::derived_from<TRes, resource>
  [[nodiscard]] const TRes* get_resource(const string_id& id) const;

  // Cook project from uncooked version
  // and write results into a memory buffer.
  static void cook(const project_uncooked& project_uncooked, bit_writer& writer);

private:
  // Used only during cooking
  explicit project() = default;

  std::unordered_map<string_id, std::unique_ptr<resource>> _resources;
};

template <typename TRes>
requires std::derived_from<TRes, resource>
const TRes* project::get_resource(const string_id& id) const
{
  using namespace eely::internal;

  auto iter = _resources.find(id);
  if (iter != _resources.end()) {
    return polymorphic_downcast<const TRes*>(iter->second.get());
  }

  return nullptr;
}
}  // namespace eely