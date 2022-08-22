#include "eely/btree/btree_node_base.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/btree/btree_node_add.h"
#include "eely/btree/btree_node_blend.h"
#include "eely/btree/btree_node_clip.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <vector>

namespace eely {
enum class btree_node_type { add, blend, clip };

static constexpr gsl::index bits_btree_node_type{4};

btree_node_base::btree_node_base(bit_reader& reader)
{
  using namespace eely::internal;

  const gsl::index children_size{reader.read(bits_btree_node_index)};
  EXPECTS(children_size <= btree_nodes_max_size);
  _children_indices.resize(children_size);

  for (gsl::index i{0}; i < children_size; ++i) {
    _children_indices[i] = reader.read(bits_btree_node_index);
  }
}

void btree_node_base::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  const gsl::index children_size{std::ssize(_children_indices)};
  EXPECTS(children_size <= btree_nodes_max_size);

  writer.write({.value = gsl::narrow<uint32_t>(children_size), .size_bits = bits_btree_node_index});

  for (gsl::index i{0}; i < children_size; ++i) {
    writer.write(
        {.value = gsl::narrow<uint32_t>(_children_indices[i]), .size_bits = bits_btree_node_index});
  };
}

const std::vector<gsl::index>& btree_node_base::get_children_indices() const
{
  return _children_indices;
}

std::vector<gsl::index>& btree_node_base::get_children_indices()
{
  return _children_indices;
}

namespace internal {
void btree_node_serialize(const btree_node_base& node, bit_writer& writer)
{
  btree_node_type type;
  if (dynamic_cast<const btree_node_add*>(&node) != nullptr) {
    type = btree_node_type::add;
  }
  else if (dynamic_cast<const btree_node_blend*>(&node) != nullptr) {
    type = btree_node_type::blend;
  }
  else if (dynamic_cast<const btree_node_clip*>(&node) != nullptr) {
    type = btree_node_type::clip;
  }
  else {
    throw std::runtime_error("Unknown btree node type for serialization");
  }

  writer.write({.value = static_cast<uint32_t>(type), .size_bits = bits_btree_node_type});

  node.serialize(writer);
}

std::unique_ptr<btree_node_base> btree_node_deserialize(bit_reader& reader)
{
  const auto type{static_cast<btree_node_type>(reader.read(bits_btree_node_type))};
  switch (type) {
    case btree_node_type::add: {
      return std::make_unique<btree_node_add>(reader);
    } break;

    case btree_node_type::blend: {
      return std::make_unique<btree_node_blend>(reader);
    } break;

    case btree_node_type::clip: {
      return std::make_unique<btree_node_clip>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown btree node type for deserialization");
    } break;
  }
}
}  // namespace internal
}  // namespace eely