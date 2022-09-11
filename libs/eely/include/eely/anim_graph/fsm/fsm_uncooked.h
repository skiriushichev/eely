#pragma once

#include "eely/anim_graph/anim_graph_base_uncooked.h"

namespace eely {
// Represents an uncooked state machine resource.
// State machine describes animated states and transitions between them.
class fsm_uncooked final : public anim_graph_base_uncooked {
  using anim_graph_base_uncooked::anim_graph_base_uncooked;
};
}  // namespace eely