#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <bit>
#include <memory>
#include <unordered_set>

namespace eely {
// Enum for all types of animation nodes.
// Can be queried using `anim_graph_node_base::get_type()`,
// and used instead of dynamics casts when checking node type is needed
// (e.g. in serialization etc.).
enum class anim_graph_node_type {
  and_logic,  // `and` is a keyword :(
  blend,
  clip,
  param_comparison,
  param,
  random,
  speed,
  state_condition,
  state_machine,
  state_transition,
  state,
  sum
};

// Common class for all animation graph nodes.
// These are definitions from which runtime nodes are created by `anim_graph_player`.
class anim_graph_node_base {
public:
  // Construct a node with specified unique ID within a graph.
  explicit anim_graph_node_base(anim_graph_node_type type, int id);

  // Construct a node from a memory buffer.
  explicit anim_graph_node_base(anim_graph_node_type type, internal::bit_reader& reader);

  virtual ~anim_graph_node_base() = default;

  // Serialize a node into a memory buffer.
  virtual void serialize(internal::bit_writer& writer) const;

  // Collect resource dependencies for this node.
  // This should only collect direct dependencies of this node, not recursively across all children.
  virtual void collect_dependencies(std::unordered_set<string_id>& /*out_dependencies*/) {}

  // Clone this node.
  [[nodiscard]] virtual std::unique_ptr<anim_graph_node_base> clone() const = 0;

  // Return this node's type.
  [[nodiscard]] anim_graph_node_type get_type() const;

  // Return this node's id unique within a graph.
  [[nodiscard]] int get_id() const;

private:
  anim_graph_node_type _type;
  int _id;
};

// Shorter name for unique pointer to a node.
using anim_graph_node_uptr = std::unique_ptr<anim_graph_node_base>;

namespace internal {
static constexpr gsl::index anim_graph_nodes_max_size{1023};
static constexpr gsl::index bits_anim_graph_nodes_size{
    std::bit_width(static_cast<size_t>(anim_graph_nodes_max_size - 1))};

static constexpr gsl::index anim_graph_max_node_id{1023};
static constexpr gsl::index bits_anim_graph_node_id{
    std::bit_width(static_cast<size_t>(anim_graph_max_node_id))};

static constexpr gsl::index bits_anim_graph_node_type{8};

// Return animation graph node read from a memory buffer.
template <>
anim_graph_node_uptr bit_reader_read(bit_reader& reader);

// Write animation graph node into a memory buffer.
// This method serializes node's type as well as its data, thus it can be recreated
// with correct type from this buffer (using `bit_reader_read`).
void bit_writer_write(bit_writer& writer, const anim_graph_node_base& node);
}  // namespace internal
}  // namespace eely