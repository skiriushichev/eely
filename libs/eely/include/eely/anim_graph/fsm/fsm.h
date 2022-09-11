#pragma once

#include "eely/anim_graph/anim_graph_base.h"
#include "eely/anim_graph/fsm/fsm_uncooked.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"

namespace eely {
// Represents a state machine resource.
// State machine describes set of animated states and transitions between them.
class fsm final : public anim_graph_base {
public:
  using anim_graph_base::anim_graph_base;
};
}  // namespace eely