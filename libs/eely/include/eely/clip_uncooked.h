#pragma once

#include "eely/bit_reader.h"
#include "eely/bit_writer.h"
#include "eely/clip_utils.h"
#include "eely/float3.h"
#include "eely/quaternion.h"
#include "eely/resource_uncooked.h"
#include "eely/string_id.h"

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

  // Aniamted track for a single joint.
  struct track final {
    // Id of a joint this track is for.
    string_id joint_id;

    // Time in seconds -> key
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
  [[nodiscard]] compression_scheme get_compression_scheme() const;

  // Set compression scheme for the clip.
  void set_compression_scheme(compression_scheme scheme);

  // Return clip's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

private:
  string_id _target_skeleton_id;
  compression_scheme _compression_scheme;
  std::vector<track> _tracks;
};
}  // namespace eely