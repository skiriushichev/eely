#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_cursor.h"
#include "eely/clip/clip_impl_base.h"
#include "eely/clip/clip_utils.h"
#include "eely/skeleton/skeleton.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace eely::internal {
// Metadata for uncompressed clips.
struct clip_metadata_none final : public clip_metadata_base {
  std::vector<joint_components> joints_components;
};

// Implementation for uncompressed clips.
class clip_impl_none final : public clip_impl_base {
public:
  explicit clip_impl_none(bit_reader& reader);

  explicit clip_impl_none(float duration_s,
                          const std::vector<clip_uncooked_track>& tracks,
                          bool is_additive,
                          const skeleton& skeleton);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] const clip_metadata_base* get_metadata() const override;

  [[nodiscard]] std::unique_ptr<clip_player_base> create_player() const override;

private:
  clip_metadata_none _metadata;
  std::vector<uint32_t> _data;
};
}  // namespace eely::internal