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
#include <unordered_set>
#include <vector>

namespace eely {
// Key in an animation track.
struct clip_uncooked_key final {
  std::optional<float3> translation;
  std::optional<quaternion> rotation;
  std::optional<float3> scale;
};

// Animated track for a single joint.
struct clip_uncooked_track final {
  // Id of a joint this track is for.
  string_id joint_id;

  // Time in seconds -> key.
  std::map<float, clip_uncooked_key> keys;
};

// Represents an uncooked animation clip.
class clip_uncooked final : public resource_uncooked {
public:
  // Construct an uncooked clip from a memory buffer.
  explicit clip_uncooked(internal::bit_reader& reader);

  // Construct an empty uncooked clip.
  explicit clip_uncooked(const string_id& id);

  void serialize(internal::bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Return id of a skeleton this clip is targeted at.
  [[nodiscard]] const string_id& get_target_skeleton_id() const;

  // Set id of a skeleton this clip is targeted at.
  void set_target_skeleton_id(string_id skeleton_id);

  // Return skeleton mask to be used when clip is cooked.
  [[nodiscard]] string_id get_skeleton_mask_id() const;

  // Set skeleton mask to be used when clip is cooked.
  // Joints with zero weight will not be in a final clip,
  // and others will have an according percentage written.
  void set_skeleton_mask_id(string_id skeleton_mask_id);

  // Return list of clip tracks.
  [[nodiscard]] const std::vector<clip_uncooked_track>& get_tracks() const;

  // Set list of clip tracks.
  void set_tracks(std::vector<clip_uncooked_track> tracks);

  // Return clip's compression scheme.
  [[nodiscard]] clip_compression_scheme get_compression_scheme() const;

  // Set compression scheme for the clip.
  void set_compression_scheme(clip_compression_scheme scheme);

  // Return clip's duration in seconds.
  [[nodiscard]] float get_duration_s() const;

private:
  string_id _target_skeleton_id;
  string_id _skeleton_mask_id;
  clip_compression_scheme _compression_scheme;
  std::vector<clip_uncooked_track> _tracks;
};

// Represents an uncooked animation clip calculated as a difference between other two
// clips, and is meant to be used additively on top of another animation.
struct clip_additive_uncooked final : public resource_uncooked {
public:
  struct range final {
    float from_s{0.0F};
    float to_s{0.0F};
  };

  // Construct an uncooked additive clip from a memory buffer.
  explicit clip_additive_uncooked(internal::bit_reader& reader);

  // Construct an empty uncooked additive clip.
  explicit clip_additive_uncooked(const string_id& id);

  void serialize(internal::bit_writer& writer) const override;

  void collect_dependencies(std::unordered_set<string_id>& out_dependencies) const override;

  // Return id of a skeleton this clip is targeted at.
  [[nodiscard]] const string_id& get_target_skeleton_id() const;

  // Set id of a skeleton this clip is targeted at.
  void set_target_skeleton_id(string_id skeleton_id);

  // Set skeleton mask to be used when additive clip is calculated.
  // Joints with zero weight will not be in a final clip,
  // and others will have an according percentage written.
  void set_skeleton_mask_id(string_id skeleton_mask_id);

  // Return skeleton mask to be used when additive clip is calculated.
  [[nodiscard]] string_id get_skeleton_mask_id() const;

  // Set id of a base clip.
  void set_base_clip_id(string_id base_clip_id);

  // Return id of a base clip.
  [[nodiscard]] const string_id& get_base_clip_id() const;

  // Set id of a source clip.
  void set_source_clip_id(string_id source_clip_id);

  // Return id of a source clip.
  [[nodiscard]] const string_id& get_source_clip_id() const;

  // Set time range of a base clip.
  // If empty, whole range will be used.
  void set_base_clip_range(const std::optional<range>& base_clip_range);

  // Return time range of a base clip.
  [[nodiscard]] const std::optional<range>& get_base_clip_range() const;

  // Set time range of a source clip.
  // If empty, whole range will be used.
  // This will also be a duration of the calculated additive clip.
  void set_source_clip_range(const std::optional<range>& source_clip_range);

  // Return time range of a source clip.
  [[nodiscard]] const std::optional<range>& get_source_clip_range() const;

  // Return clip's compression scheme.
  [[nodiscard]] clip_compression_scheme get_compression_scheme() const;

  // Set compression scheme for the clip.
  void set_compression_scheme(clip_compression_scheme scheme);

private:
  string_id _target_skeleton_id;
  string_id _skeleton_mask_id;
  string_id _base_clip_id;
  string_id _source_clip_id;
  std::optional<range> _base_clip_range;
  std::optional<range> _source_clip_range;
  clip_compression_scheme _compression_scheme;
};
}  // namespace eely