#pragma once

#include "eely/clip_player_base.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"

#include <cstdint>
#include <span>

namespace eely {
// Player for uncompressed clips.
class clip_player_uncompressed final : public clip_player_base {
public:
  // Construct player for specified uncompressed animation data and skeleton.
  explicit clip_player_uncompressed(std::span<const uint32_t> data,
                                    float duration_s,
                                    const skeleton& skeleton);

private:
  void play_impl(float time_s, skeleton_pose& out_pose) override;

  const std::span<const uint32_t> _data;
  const skeleton& _skeleton;

  cursor _cursor;
};

}  // namespace eely