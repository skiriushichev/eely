#pragma once

#include "eely/anim_graph/anim_graph_base.h"
#include "eely/anim_graph/btree/btree_uncooked.h"
#include "eely/base/bit_reader.h"
#include "eely/project/project.h"

namespace eely {
// Represents a blendtree resource.
// Blend trees describe how animations are combined in a final pose based on input parameters.
// Animations are not necessarily clips, but also state machines, IK, physics etc.
class btree final : public anim_graph_base {
public:
  // Construct blendtree from a memory buffer.
  explicit btree(const project& project, bit_reader& reader);

  // Construct blendtree from its uncooked counterpart.
  explicit btree(const project& project, const btree_uncooked& uncooked);
};
}  // namespace eely