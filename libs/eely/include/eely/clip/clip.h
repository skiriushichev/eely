#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_impl_base.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"

#include <memory>

namespace eely {
// Represents a cooked animation clip that can be played.
class clip final : public resource {
public:
  // Construct clip from a memory buffer.
  explicit clip(bit_reader& reader);

  // Construct clip from an uncooked counterpart.
  explicit clip(const project& project, const clip_uncooked& uncooked);

  // Serialize clip into a memory buffer.
  void serialize(bit_writer& writer) const override;

  // Get clip's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

  // Create a player for this clip.
  // Players should not outlive the clip.
  [[nodiscard]] std::unique_ptr<clip_player_base> create_player() const;

private:
  std::unique_ptr<internal::clip_impl_base> _impl;
};
}  // namespace eely