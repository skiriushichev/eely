#include "eely/anim_graph/btree/btree.h"

#include "eely/anim_graph/anim_graph_base.h"
#include "eely/anim_graph/btree/btree_uncooked.h"
#include "eely/base/bit_reader.h"
#include "eely/project/project.h"

namespace eely {
btree::btree(const project& project, bit_reader& reader) : anim_graph_base(project, reader) {}

btree::btree(const project& project, const btree_uncooked& uncooked)
    : anim_graph_base(project, uncooked)
{
}
}  // namespace eely