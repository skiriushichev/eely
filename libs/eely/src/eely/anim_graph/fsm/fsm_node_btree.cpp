#include "eely/anim_graph/fsm/fsm_node_btree.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/fsm/fsm_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
fsm_node_btree::fsm_node_btree(bit_reader& reader)
    : fsm_node_base(reader), _btree_id{string_id_deserialize(reader)}
{
}

void fsm_node_btree::serialize(bit_writer& writer) const
{
  fsm_node_base::serialize(writer);

  string_id_serialize(_btree_id, writer);
}

std::unique_ptr<anim_graph_node_base> fsm_node_btree::clone() const
{
  return std::make_unique<fsm_node_btree>(*this);
}

void fsm_node_btree::collect_dependencies(std::unordered_set<string_id>& out_dependencies)
{
  out_dependencies.insert(_btree_id);
}

const string_id& fsm_node_btree::get_btree_id() const
{
  return _btree_id;
}

void fsm_node_btree::set_btree_id(string_id btree_id)
{
  _btree_id = std::move(btree_id);
}
}  // namespace eely