#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <gsl/util>

#include <memory>
#include <unordered_set>
#include <vector>

namespace eely {
// Common class for all blend tree nodes, derived classes describe specific types of nodes.
// These are the definitions from which runtime nodes are created by `btree_player`.
class btree_node_base {
public:
  // Construct empty node.
  explicit btree_node_base() = default;

  // Construct node from a memory buffer.
  explicit btree_node_base(bit_reader& reader);

  virtual ~btree_node_base() = default;

  // Serialize node into a memory buffer.
  virtual void serialize(bit_writer& writer) const;

  // Construct a copy of this node.
  [[nodiscard]] virtual std::unique_ptr<btree_node_base> clone() const = 0;

  // Collect resource dependencies for this node.
  virtual void collect_dependencies(std::unordered_set<string_id>& /*out_dependencies*/) {}

  // Return read-only vector of children indices.
  [[nodiscard]] const std::vector<gsl::index>& get_children_indices() const;

  // Return vector of children indices.
  [[nodiscard]] std::vector<gsl::index>& get_children_indices();

private:
  std::vector<gsl::index> _children_indices;
};

// Somewhat shorter name for a unique pointer to a btree node.
using btree_node_uptr = std::unique_ptr<btree_node_base>;

namespace internal {
static constexpr gsl::index btree_nodes_max_size{255};
static constexpr gsl::index bits_btree_node_index{8};

// Serialize node into a memory buffer.
// This method serializes node's type as well as its data,
// thus it can be recreated with correct type from this buffer (using `btree_node_deserialize`).
void btree_node_serialize(const btree_node_base& node, bit_writer& writer);

// Deserialize node previously saved via `btree_node_serialize`.
btree_node_uptr btree_node_deserialize(bit_reader& reader);
}  // namespace internal
}  // namespace eely