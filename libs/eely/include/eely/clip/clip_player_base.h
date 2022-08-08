#pragma once

#include "eely/skeleton/skeleton_pose.h"

namespace eely {
// Base class for all clip players.
// Players produce poses from clips.
class clip_player_base {
public:
  virtual ~clip_player_base() = default;

  // Get duration of a clip being played.
  [[nodiscard]] virtual float get_duration_s() = 0;

  // Calculate skeleton pose at specified absolute time.
  // Time should be within [0.0F; duration] interval.
  virtual void play(float time_s, skeleton_pose& out_pose) = 0;
};
}  // namespace eely