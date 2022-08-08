#pragma once

#include "eely/clip/clip_cursor.h"
#include "eely/clip/clip_impl_none.h"
#include "eely/clip/clip_player_base.h"
#include "eely/skeleton/skeleton_pose.h"

#include <cstdint>
#include <span>

namespace eely::internal {
// Player for uncompressed clips.
class clip_player_none final : public clip_player_base {
public:
  explicit clip_player_none(const clip_metadata_none& metadata, std::span<const uint32_t> data);

  [[nodiscard]] float get_duration_s() override;

  void play(float time_s, skeleton_pose& out_pose) override;

private:
  const clip_metadata_none& _metadata;
  const std::span<const uint32_t> _data;

  cursor _cursor;
};
}  // namespace eely::internal