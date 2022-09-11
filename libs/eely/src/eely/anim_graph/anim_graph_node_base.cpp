#include "eely/anim_graph/anim_graph_node_base.h"

#include "eely/anim_graph/btree/btree_node_add.h"
#include "eely/anim_graph/btree/btree_node_blend.h"
#include "eely/anim_graph/btree/btree_node_btree.h"
#include "eely/anim_graph/btree/btree_node_clip.h"
#include "eely/anim_graph/btree/btree_node_fsm.h"
#include "eely/anim_graph/fsm/fsm_node_btree.h"
#include "eely/anim_graph/fsm/fsm_node_fsm.h"
#include "eely/anim_graph/fsm/fsm_node_transition.h"
#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <gsl/narrow>
#include <gsl/util>

#include <memory>
#include <unordered_set>
#include <vector>

namespace eely {
anim_graph_node_base::anim_graph_node_base(bit_reader& reader)
{
  using namespace eely::internal;

  const gsl::index children_size{reader.read(bits_anim_graph_node_index)};
  EXPECTS(children_size <= anim_graph_nodes_max_size);
  _children_indices.resize(children_size);

  for (gsl::index i{0}; i < children_size; ++i) {
    _children_indices[i] = reader.read(bits_anim_graph_node_index);
  }
}

void anim_graph_node_base::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  const gsl::index children_size{std::ssize(_children_indices)};
  EXPECTS(children_size <= anim_graph_nodes_max_size);

  writer.write(
      {.value = gsl::narrow<uint32_t>(children_size), .size_bits = bits_anim_graph_node_index});

  for (gsl::index i{0}; i < children_size; ++i) {
    writer.write({.value = gsl::narrow<uint32_t>(_children_indices[i]),
                  .size_bits = bits_anim_graph_node_index});
  };
}

const std::vector<gsl::index>& anim_graph_node_base::get_children_indices() const
{
  return _children_indices;
}

std::vector<gsl::index>& anim_graph_node_base::get_children_indices()
{
  return _children_indices;
}

namespace internal {
enum class anim_graph_node_type {
  btree_add,
  btree_blend,
  btree_btree,
  btree_clip,
  btree_fsm,
  fsm_btree,
  fsm_fsm,
  fsm_transition
};

static constexpr gsl::index bits_anim_graph_node_type{4};

void anim_graph_node_serialize(const anim_graph_node_base& node, bit_writer& writer)
{
  // TODO: use enum + polymorphic_downcast

  anim_graph_node_type type;
  if (dynamic_cast<const btree_node_add*>(&node) != nullptr) {
    type = anim_graph_node_type::btree_add;
  }
  else if (dynamic_cast<const btree_node_blend*>(&node) != nullptr) {
    type = anim_graph_node_type::btree_blend;
  }
  else if (dynamic_cast<const btree_node_btree*>(&node) != nullptr) {
    type = anim_graph_node_type::btree_btree;
  }
  else if (dynamic_cast<const btree_node_clip*>(&node) != nullptr) {
    type = anim_graph_node_type::btree_clip;
  }
  else if (dynamic_cast<const btree_node_fsm*>(&node) != nullptr) {
    type = anim_graph_node_type::btree_fsm;
  }
  else if (dynamic_cast<const fsm_node_btree*>(&node) != nullptr) {
    type = anim_graph_node_type::fsm_btree;
  }
  else if (dynamic_cast<const fsm_node_fsm*>(&node) != nullptr) {
    type = anim_graph_node_type::fsm_fsm;
  }
  else if (dynamic_cast<const fsm_node_transition*>(&node) != nullptr) {
    type = anim_graph_node_type::fsm_transition;
  }
  else {
    throw std::runtime_error("Unknown animation graph node type for serialization");
  }

  writer.write({.value = static_cast<uint32_t>(type), .size_bits = bits_anim_graph_node_type});

  node.serialize(writer);
}

anim_graph_node_uptr anim_graph_node_deserialize(bit_reader& reader)
{
  const auto type{static_cast<anim_graph_node_type>(reader.read(bits_anim_graph_node_type))};
  switch (type) {
    case anim_graph_node_type::btree_add: {
      return std::make_unique<btree_node_add>(reader);
    } break;

    case anim_graph_node_type::btree_blend: {
      return std::make_unique<btree_node_blend>(reader);
    } break;

    case anim_graph_node_type::btree_btree: {
      return std::make_unique<btree_node_btree>(reader);
    } break;

    case anim_graph_node_type::btree_clip: {
      return std::make_unique<btree_node_clip>(reader);
    } break;

    case anim_graph_node_type::btree_fsm: {
      return std::make_unique<btree_node_fsm>(reader);
    } break;

    case anim_graph_node_type::fsm_btree: {
      return std::make_unique<fsm_node_btree>(reader);
    } break;

    case anim_graph_node_type::fsm_fsm: {
      return std::make_unique<fsm_node_fsm>(reader);
    } break;

    case anim_graph_node_type::fsm_transition: {
      return std::make_unique<fsm_node_transition>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown animation graph node type for deserialization");
    } break;
  }
}
}  // namespace internal
}  // namespace eely