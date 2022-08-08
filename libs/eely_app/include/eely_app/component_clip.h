#pragma once

#include <eely/clip/clip_player_base.h>

#include <memory>

namespace eely {
struct component_clip final {
  std::unique_ptr<clip_player_base> player;
  float speed{1.0F};
  float play_time_s{0.0F};
};
}  // namespace eely