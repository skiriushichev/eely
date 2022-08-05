#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/clip_player_base.h"
#include "eely/clip_uncooked.h"
#include "eely/clip_utils.h"
#include "eely/project.h"
#include "eely/resource.h"
#include "eely/skeleton.h"

#include <gsl/util>

#include <cstdint>
#include <memory>
#include <vector>

namespace eely {
// Represents a cooked animation clip that can be played.
class clip final : public resource {
public:
  // Construct clip from a memory buffer.
  explicit clip(const project& project, bit_reader& reader);

  // Construct clip from an uncooked counterpart.
  explicit clip(const project& project, const clip_uncooked& uncooked);

  // Serialize clip into a memory buffer.
  void serialize(bit_writer& writer) const override;

  // Get clip's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Get clip's metadata.
  [[nodiscard]] std::variant<clip_metadata, clip_metadata_compressed_fixed> get_metadata() const;

  // Create a player for this clip.
  // Players should not outlive the clip.
  [[nodiscard]] std::unique_ptr<clip_player_base> create_player() const;

private:
  struct uncompressed_clip final {
    std::vector<uint32_t> data;
    clip_metadata metadata;
  };

  struct compressed_fixed_clip final {
    std::vector<uint16_t> data;
    clip_metadata_compressed_fixed metadata;
  };

  using clip_variant = std::variant<uncompressed_clip, compressed_fixed_clip>;

  const skeleton& _skeleton;
  clip_variant _clip_variant;
};
}  // namespace eely