#pragma once

#include "eely/clip_player_base.h"
#include "eely/clip_utils.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"

#include <cstdint>
#include <span>

namespace eely {
// Player for uncompressed clips.
class clip_player_uncompressed final : public clip_player_base {
public:
  // Construct player for specified uncompressed animation.
  explicit clip_player_uncompressed(std::span<const uint32_t> data, const clip_metadata& metadata);

  [[nodiscard]] float get_duration_s() override;

  void play(float time_s, skeleton_pose& out_pose) override;

private:
  const std::span<const uint32_t> _data;
  const clip_metadata& _metadata;

  cursor _cursor;
};

}  // namespace eely