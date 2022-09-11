#pragma once

#include "eely/anim_graph/anim_graph_base_uncooked.h"

namespace eely {
// Represents an uncooked blendtree resource.
// Blend trees describe how animations are combined in a final pose based on input parameters.
// Animations are not necessarily clips, but also state machines, IK, physics etc.
class btree_uncooked final : public anim_graph_base_uncooked {
  using anim_graph_base_uncooked::anim_graph_base_uncooked;
};
}  // namespace eely