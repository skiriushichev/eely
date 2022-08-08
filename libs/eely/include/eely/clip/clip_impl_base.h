#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_player_base.h"

#include <memory>

namespace eely::internal {
// Base metadata used by all clips no matter the scheme.
struct clip_metadata_base {
  virtual ~clip_metadata_base() = default;

  // Clip duration in seconds.
  float duration_s{0.0F};
};

// Base interface for clip implementations, one for each compression scheme.
class clip_impl_base {
public:
  virtual ~clip_impl_base() = default;

  virtual void serialize(bit_writer& writer) const = 0;

  [[nodiscard]] virtual const clip_metadata_base* get_metadata() const = 0;

  [[nodiscard]] virtual std::unique_ptr<clip_player_base> create_player() const = 0;
};
}  // namespace eely::internal