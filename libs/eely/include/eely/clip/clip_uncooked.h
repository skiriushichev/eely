#pragma once

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/clip/clip_compression_scheme.h"
#include "eely/math/float3.h"
#include "eely/math/quaternion.h"
#include "eely/project/resource_uncooked.h"

#include <map>
#include <optional>
#include <vector>

namespace eely {
// Represents an uncooked animation clip.
class clip_uncooked final : public resource_uncooked {
public:
  // Key in an animation track.
  struct key final {
    std::optional<float3> translation;
    std::optional<quaternion> rotation;
    std::optional<float3> scale;
  };

  // Animated track for a single joint.
  struct track final {
    // Id of a joint this track is for.
    string_id joint_id;

    // Time in seconds -> key.
    std::map<float, key> keys;
  };

  // Construct an uncooked clip from a memory buffer.
  explicit clip_uncooked(bit_reader& reader);

  // Construct an empty uncooked clip.
  explicit clip_uncooked(const string_id& id);

  // Serialize clip into memory buffer.
  void serialize(bit_writer& writer) const override;

  // Collect ids of all dependencies for this resource.
  void collect_dependencies(std::vector<string_id>& out_dependencies) const override;

  // Return id of a skeleton this clip is targeted at.
  [[nodiscard]] const string_id& get_target_skeleton_id() const;

  // Set id of a skeleton this clip is targeted at.
  void set_target_skeleton_id(string_id skeleton_id);

  // Return list of clip tracks.
  [[nodiscard]] const std::vector<track>& get_tracks() const;

  // Set list of clip tracks.
  void set_tracks(std::vector<track> tracks);

  // Return clip's compression scheme.
  [[nodiscard]] clip_compression_scheme get_compression_scheme() const;

  // Set compression scheme for the clip.
  void set_compression_scheme(clip_compression_scheme scheme);

  // Return clip's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

private:
  string_id _target_skeleton_id;
  clip_compression_scheme _compression_scheme;
  std::vector<track> _tracks;
};
}  // namespace eely