#include "eely/anim_graph/btree/btree_node_fsm.h"

#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/btree/btree_node_base.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"

#include <memory>
#include <unordered_set>

namespace eely {
btree_node_fsm::btree_node_fsm(bit_reader& reader)
    : btree_node_base(reader), _fsm_id{string_id_deserialize(reader)}
{
}

void btree_node_fsm::serialize(bit_writer& writer) const
{
  btree_node_base::serialize(writer);

  string_id_serialize(_fsm_id, writer);
}

std::unique_ptr<anim_graph_node_base> btree_node_fsm::clone() const
{
  return std::make_unique<btree_node_fsm>(*this);
}

void btree_node_fsm::collect_dependencies(std::unordered_set<string_id>& out_dependencies)
{
  out_dependencies.insert(_fsm_id);
}

const string_id& btree_node_fsm::get_fsm_id() const
{
  return _fsm_id;
}

void btree_node_fsm::set_fsm_id(string_id fsm_id)
{
  _fsm_id = std::move(fsm_id);
}
}  // namespace eely