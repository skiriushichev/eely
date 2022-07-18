#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/clip_player_base.h"
#include "eely/clip_uncooked.h"
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

  // Create a player for this clip.
  // Players should not outlive the clip.
  [[nodiscard]] std::unique_ptr<clip_player_base> create_player() const;

private:
  const skeleton& _skeleton;
  std::vector<uint32_t> _data;
  float _duration_s;
};
}  // namespace eely