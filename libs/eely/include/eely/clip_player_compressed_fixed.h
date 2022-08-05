#pragma once

#include "eely/clip_player_base.h"
#include "eely/clip_utils.h"
#include "eely/skeleton.h"
#include "eely/skeleton_pose.h"

#include <cstdint>
#include <span>

namespace eely {
// Player for uncompressed clips.
class clip_player_compressed_fixed final : public clip_player_base {
public:
  // Construct player for specified compressed animation.
  explicit clip_player_compressed_fixed(std::span<const uint16_t> data,
                                        const clip_metadata_compressed_fixed& metadata);

  [[nodiscard]] float get_duration_s() override;

  void play(float time_s, skeleton_pose& out_pose) override;

private:
  const std::span<const uint16_t> _data;
  const clip_metadata_compressed_fixed& _metadata;

  cursor _cursor;
};

}  // namespace eely