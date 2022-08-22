#include "eely/btree/btree_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/btree/btree_node_base.h"
#include "eely/project/resource_uncooked.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace eely {
btree_uncooked::btree_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  using namespace eely::internal;

  const gsl::index nodes_size{reader.read(bits_btree_node_index)};
  _nodes.reserve(nodes_size);

  for (gsl::index i{0}; i < nodes_size; ++i) {
    _nodes.push_back(btree_node_deserialize(reader));
  }

  const bool has_root_index{reader.read(1) == 1};
  if (has_root_index) {
    _root_node_index = reader.read(bits_btree_node_index);
  }

  _skeleton_id = string_id_deserialize(reader);
}

btree_uncooked::btree_uncooked(const string_id& id) : resource_uncooked(id) {}

void btree_uncooked::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  const gsl::index nodes_size{std::ssize(_nodes)};
  writer.write({.value = gsl::narrow<uint32_t>(nodes_size), .size_bits = bits_btree_node_index});

  for (gsl::index i{0}; i < nodes_size; ++i) {
    EXPECTS(_nodes[i] != nullptr);
    btree_node_serialize(*_nodes[i], writer);
  }

  if (_root_node_index.has_value()) {
    writer.write({.value = 1, .size_bits = 1});
    writer.write({.value = gsl::narrow_cast<uint32_t>(_root_node_index.value()),
                  .size_bits = bits_btree_node_index});
  }
  else {
    writer.write({.value = 0, .size_bits = 1});
  }

  string_id_serialize(_skeleton_id, writer);
}

void btree_uncooked::collect_dependencies(std::unordered_set<string_id>& out_dependencies) const
{
  for (const btree_node_uptr& node : _nodes) {
    node->collect_dependencies(out_dependencies);
  }
}

std::vector<std::unique_ptr<btree_node_base>>& btree_uncooked::get_nodes()
{
  return _nodes;
}

const std::vector<std::unique_ptr<btree_node_base>>& btree_uncooked::get_nodes() const
{
  return _nodes;
}

std::optional<gsl::index> btree_uncooked::get_root_node_index() const
{
  return _root_node_index;
}

void btree_uncooked::set_root_node_index(const gsl::index index)
{
  _root_node_index = index;
}

void btree_uncooked::set_skeleton_id(string_id id)
{
  _skeleton_id = std::move(id);
}

const string_id& btree_uncooked::get_skeleton_id() const
{
  return _skeleton_id;
}
}  // namespace eely