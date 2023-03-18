#pragma once

#include <eely/anim_graph/anim_graph_player.h>
#include <eely/params/params.h>

#include <memory>

namespace eely {
struct component_anim_graph final {
  std::unique_ptr<anim_graph_player> player;
  params* params{nullptr};
};
}  // namespace eely