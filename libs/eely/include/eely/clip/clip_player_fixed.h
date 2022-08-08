#pragma once

#include "eely/clip/clip_cursor.h"
#include "eely/clip/clip_impl_fixed.h"
#include "eely/clip/clip_player_base.h"
#include "eely/skeleton/skeleton_pose.h"

#include <cstdint>
#include <span>

namespace eely::internal {
// Player for clips compressed with `clip_compression_scheme::fixed`.
class clip_player_fixed final : public clip_player_base {
public:
  explicit clip_player_fixed(const clip_metadata_fixed& metadata, std::span<const uint16_t> data);

  [[nodiscard]] float get_duration_s() override;

  void play(float time_s, skeleton_pose& out_pose) override;

private:
  const clip_metadata_fixed& _metadata;
  const std::span<const uint16_t> _data;

  cursor _cursor;
};
}  // namespace eely::internal