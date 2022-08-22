#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_impl_base.h"
#include "eely/clip/clip_uncooked.h"

#include <acl/compression/compress.h>
#include <acl/core/ansi_allocator.h>
#include <acl/decompression/decompress.h>

#include <gsl/util>

#include <cstdint>
#include <memory>
#include <vector>

namespace eely::internal {
// Metadata for clips compressed with `clip_compression_scheme::acl`.
struct clip_metadata_acl final : public clip_metadata_base {
  gsl::index shallow_joint_index{std::numeric_limits<gsl::index>::max()};
};

// Implementation for clips clips compressed with `clip_compression_scheme::acl`.
class clip_impl_acl final : public clip_impl_base {
public:
  explicit clip_impl_acl(bit_reader& reader);

  explicit clip_impl_acl(float duration_s,
                         const std::vector<clip_uncooked_track>& tracks,
                         bool is_additive,
                         const skeleton& skeleton);

  void serialize(bit_writer& writer) const override;

  [[nodiscard]] const clip_metadata_base* get_metadata() const override;

  [[nodiscard]] std::unique_ptr<clip_player_base> create_player() const override;

private:
  clip_metadata_acl _metadata;

  // Unique ptr for `std::aligned_alloc`/`std::free` pair,
  // since ACL has alignment requirements for compressed tracks
  std::unique_ptr<uint8_t, decltype(&std::free)> _acl_compressed_tracks_storage{nullptr, nullptr};

  acl::ansi_allocator _acl_allocator;
  const acl::compressed_tracks* _acl_compressed_tracks;
};
}  // namespace eely::internal