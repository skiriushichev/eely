#include "eely/anim_graph/btree/btree_node_btree.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/btree/btree_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
btree_node_btree::btree_node_btree(bit_reader& reader)
    : btree_node_base(reader), _btree_id{string_id_deserialize(reader)}
{
}

void btree_node_btree::serialize(bit_writer& writer) const
{
  btree_node_base::serialize(writer);

  string_id_serialize(_btree_id, writer);
}

std::unique_ptr<anim_graph_node_base> btree_node_btree::clone() const
{
  return std::make_unique<btree_node_btree>(*this);
}

void btree_node_btree::collect_dependencies(std::unordered_set<string_id>& out_dependencies)
{
  out_dependencies.insert(_btree_id);
}

const string_id& btree_node_btree::get_btree_id() const
{
  return _btree_id;
}

void btree_node_btree::set_btree_id(string_id btree_id)
{
  _btree_id = std::move(btree_id);
}
}  // namespace eely