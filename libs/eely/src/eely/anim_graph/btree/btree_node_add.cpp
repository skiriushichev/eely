#include "eely/anim_graph/btree/btree_node_add.h"

#include "eely/anim_graph/anim_graph_node_base.h"

#include <memory>

namespace eely {
std::unique_ptr<anim_graph_node_base> btree_node_add::clone() const
{
  return std::make_unique<btree_node_add>(*this);
}
}  // namespace eely