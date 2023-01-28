#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/resource_uncooked.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
// Represents an uncooked animation graph, which is traversed to produce skeleton poses.
// Contains different types of nodes that calculate poses and combine them,
// such as playing clips, blending poses, running state machines etc.
class anim_graph_uncooked final : public resource_uncooked {
public:
  // Construct an uncooked animation graph resource from a memory buffer.
  explicit anim_graph_uncooked(internal::bit_reader& reader);

  // Construct an empty uncooked animation graph resource.
  explicit anim_graph_uncooked(const string_id& id);

  explicit anim_graph_uncooked(const anim_graph_uncooked& other);

  explicit anim_graph_uncooked(anim_graph_uncooked&& other) noexcept;

  ~anim_graph_uncooked() override = default;

  anim_graph_uncooked& operator=(const anim_graph_uncooked& other);

  anim_graph_uncooked& operator=(anim_graph_uncooked&& other) noexcept;

  void serialize(internal::bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Get id of a skeleton this graph is for.
  [[nodiscard]] const string_id& get_skeleton_id() const;

  // Set id of a skeleton this graph is for.
  void set_skeleton_id(string_id id);

  // Get list of graph nodes.
  [[nodiscard]] const std::vector<anim_graph_node_uptr>& get_nodes() const;

  // Add node of specified type and return reference to it.
  template <typename T>
  T& add_node();

  // Get id of a node from which traversal starts.
  // If empty, first node in a list will be used.
  [[nodiscard]] std::optional<int> get_root_node_id() const;

  // Set id of a node from which traversal starts.
  // If empty, first node in a list will be used.
  void set_root_node_id(std::optional<int> id);

private:
  // Generate id for a new node.
  [[nodiscard]] int generate_node_id() const;

  string_id _skeleton_id;
  std::vector<anim_graph_node_uptr> _nodes;
  std::optional<int> _root_node_id;
};

// Implementation

template <typename T>
T& anim_graph_uncooked::add_node()
{
  int id{generate_node_id()};
  std::unique_ptr<T> node_uptr{std::make_unique<T>(id)};
  T* node = node_uptr.get();
  _nodes.push_back(std::move(node_uptr));
  return *node;
}
}  // namespace eely