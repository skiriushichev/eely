#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"
#include "eely/project/resource_uncooked.h"

#include <gsl/util>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
// Represents an uncooked blend tree resource.
// Blend trees describe how animations are combined in a final pose based on input parameters.
// Animations are not necessarily clips, but also IK, physics etc.
class btree_uncooked final : public resource_uncooked {
public:
  // Construct blend tree from a memory buffer.
  explicit btree_uncooked(bit_reader& reader);

  // Construct empty blend tree with specified id.
  explicit btree_uncooked(const string_id& id);

  void serialize(bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Set id of a skeleton the blend tree is for.
  void set_skeleton_id(string_id id);

  // Get id of a skeleton the blend tree is for.
  [[nodiscard]] const string_id& get_skeleton_id() const;

  // Return tree's nodes.
  [[nodiscard]] std::vector<btree_node_uptr>& get_nodes();

  // Return read-only tree's nodes.
  [[nodiscard]] const std::vector<btree_node_uptr>& get_nodes() const;

  // Return index of a root node.
  // This is the index from which execution starts.
  [[nodiscard]] std::optional<gsl::index> get_root_node_index() const;

  // Set index of a root node.
  // This is the index from which execution starts.
  void set_root_node_index(gsl::index index);

private:
  std::vector<btree_node_uptr> _nodes;
  std::optional<gsl::index> _root_node_index;
  string_id _skeleton_id;
};
}  // namespace eely