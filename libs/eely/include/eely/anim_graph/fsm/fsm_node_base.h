#pragma once

#include "eely/anim_graph/anim_graph_node_base.h"

namespace eely {
// Base class for all state machine nodes.
class fsm_node_base : public anim_graph_node_base {
  using anim_graph_node_base::anim_graph_node_base;
};
}  // namespace eely