#pragma once

#include "eely/clip/clip_impl_acl.h"
#include "eely/clip/clip_player_base.h"
#include "eely/skeleton/skeleton_pose.h"

#include <acl/core/compressed_tracks.h>
#include <acl/decompression/decompress.h>

namespace eely::internal {
// Player for clips compressed with `clip_compression_scheme::acl`.
class clip_player_acl final : public clip_player_base {
public:
  explicit clip_player_acl(const clip_metadata_acl& metadata,
                           const acl::compressed_tracks& acl_compressed_tracks);

  [[nodiscard]] float get_duration_s() override;

  void play(float time_s, skeleton_pose& out_pose) override;

private:
  const clip_metadata_acl& _metadata;
  acl::decompression_context<acl::default_transform_decompression_settings> _decompression_context;
};

}  // namespace eely::internal