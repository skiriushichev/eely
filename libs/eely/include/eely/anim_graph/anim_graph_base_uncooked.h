#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/resource_uncooked.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
// Base uncooked version for animation graph resources: blendtrees and state machines.
// Both of them can be represented a a graph of nodes,
// so it's convenvient to have a base class with common functionality.
class anim_graph_base_uncooked : public resource_uncooked {
public:
  // Construct resource from memory.
  explicit anim_graph_base_uncooked(bit_reader& reader);

  // Construct empty resource with specified id.
  explicit anim_graph_base_uncooked(string_id id);

  void serialize(bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Set id of a skeleton this resource is for.
  void set_skeleton_id(string_id id);

  // Get id of a skelton this resource if for.
  [[nodiscard]] const string_id& get_skeleton_id() const;

  // Get vector of graph nodes.
  [[nodiscard]] std::vector<anim_graph_node_uptr>& get_nodes();

  // Get read-only vector of graph nodes.
  [[nodiscard]] const std::vector<anim_graph_node_uptr>& get_nodes() const;

  // Return index of a root node.
  // This is the index from which execution starts.
  // If not set, first node will be used as a root.
  [[nodiscard]] std::optional<gsl::index> get_root_node_index() const;

  // Set index of a root node.
  // This is the index from which execution starts.
  // If not set, first node will be used as a root.
  void set_root_node_index(std::optional<gsl::index> index);

private:
  std::vector<anim_graph_node_uptr> _nodes;
  std::optional<gsl::index> _root_node_index;
  string_id _skeleton_id;
};
}  // namespace eely