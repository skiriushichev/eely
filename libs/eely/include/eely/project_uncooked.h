#pragma once

#include "eely/axis_system.h"
#include "eely/base_utils.h"
#include "eely/graph.h"
#include "eely/measurement_unit.h"
#include "eely/resource_uncooked.h"
#include "eely/string_id.h"

#include <unordered_map>

namespace eely {
// Represents a set of uncooked resources used in an application.
class project_uncooked final {
public:
  // Construct a project from a memory buffer.
  explicit project_uncooked(bit_reader& reader);

  // Construct empty project with specified measurement unit and axis system.
  explicit project_uncooked(measurement_unit measurement_unit = measurement_unit::meters,
                            axis_system axis_system = axis_system::y_up_x_right_z_forward);

  ~project_uncooked() = default;

  project_uncooked(const project_uncooked&) = delete;
  project_uncooked(project_uncooked&&) = delete;

  project_uncooked& operator=(const project_uncooked&) = delete;
  project_uncooked& operator=(project_uncooked&&) = delete;

  // Serialize project into a memory buffer.
  void serialize(bit_writer& writer);

  // Get measurement unit used in project.
  [[nodiscard]] measurement_unit get_measurement_unit() const;

  // Get axis system used in a project.
  [[nodiscard]] axis_system get_axis_system() const;

  // Invoke specified function for every resource.
  // For every resource, all its dependencies are handled first.
  template <typename TFn>
  void for_each_resource_topological(const TFn& fn) const;

  // Get resource with specified id and type.
  // Return `nullptr` if there is no such resource.
  template <typename TRes>
  requires std::derived_from<TRes, resource_uncooked>
  [[nodiscard]] const TRes* get_resource(const string_id& id) const;

  // Add resource to the project, if there was no such resource before.
  // If there was such a resource, override it with the new one.
  // Previous version of this resource will no longer be valid.
  template <typename TRes>
  requires std::derived_from<TRes, resource_uncooked>
  void set_resource(std::unique_ptr<TRes> resource);

private:
  measurement_unit _measurement_unit;
  axis_system _axis_system;

  std::unordered_map<string_id, std::unique_ptr<resource_uncooked>> _resources;
};

// Implementation

template <typename TFn>
void project_uncooked::for_each_resource_topological(const TFn& fn) const
{
  graph<const resource_uncooked*> resources_graph;

  std::unordered_map<string_id, gsl::index> resource_id_to_vertex_id;
  for (const auto& [id, r] : _resources) {
    const auto vertex{resources_graph.add_vertex(r.get())};
    resource_id_to_vertex_id[id] = vertex.id;
  }

  std::vector<string_id> dependencies_storage;
  for (const auto& vertex : resources_graph.get_vertices()) {
    const resource_uncooked* r{vertex.data};
    r->collect_dependencies(dependencies_storage);

    for (const string_id& dependency : dependencies_storage) {
      resources_graph.add_edge(vertex.id, resource_id_to_vertex_id[dependency]);
    }

    dependencies_storage.clear();
  }

  std::vector<const resource_uncooked*> sorted_resources;
  auto sorted_resources_inserter{std::back_inserter(sorted_resources)};
  const bool traversed = graph_topological_traversal(resources_graph, sorted_resources_inserter);
  Expects(traversed);

  for (const resource_uncooked* r : sorted_resources) {
    fn(r);
  }
}

template <typename TRes>
requires std::derived_from<TRes, resource_uncooked>
const TRes* project_uncooked::get_resource(const string_id& id) const
{
  auto iter = _resources.find(id);
  if (iter != _resources.end()) {
    return polymorphic_downcast<const TRes*>(iter->second.get());
  }

  return nullptr;
}

template <typename TRes>
requires std::derived_from<TRes, resource_uncooked>
void project_uncooked::set_resource(std::unique_ptr<TRes> resource)
{
  const string_id& id{resource->get_id()};
  _resources[id] = std::move(resource);
}
}  // namespace eely