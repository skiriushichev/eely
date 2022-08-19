#include "eely/clip/clip_player_acl.h"

#include "eely/clip/clip_impl_acl.h"
#include "eely/skeleton/skeleton_pose.h"

#include <acl/core/compressed_tracks.h>
#include <acl/core/track_writer.h>
#include <acl/decompression/decompress.h>

namespace eely::internal {
struct acl_output_writer final : public acl::track_writer {
  skeleton_pose* pose{nullptr};

  void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation) const
  {
    quaternion q;
    rtm::quat_store(rotation, &q.x);
    pose->sequence_set_rotation_joint_space(track_index, q);
  }

  void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation) const
  {
    float3 v;
    rtm::vector_store3(translation, &v.x);
    pose->sequence_set_translation_joint_space(track_index, v);
  }

  void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale) const
  {
    float3 v;
    rtm::vector_store3(scale, &v.x);
    pose->sequence_set_scale_joint_space(track_index, v);
  }

  static constexpr acl::default_sub_track_mode get_default_rotation_mode()
  {
    return acl::default_sub_track_mode::skipped;
  }
  static constexpr acl::default_sub_track_mode get_default_translation_mode()
  {
    return acl::default_sub_track_mode::skipped;
  }
  static constexpr acl::default_sub_track_mode get_default_scale_mode()
  {
    return acl::default_sub_track_mode::skipped;
  }
};

clip_player_acl::clip_player_acl(const clip_metadata_acl& metadata,
                                 const acl::compressed_tracks& acl_compressed_tracks)
    : _metadata{metadata}
{
  const bool init_result{_decompression_context.initialize(acl_compressed_tracks)};
  EXPECTS(init_result);
}

float clip_player_acl::get_duration_s()
{
  return _metadata.duration_s;
}

void clip_player_acl::play(const float time_s, skeleton_pose& out_pose)
{
  if (_metadata.is_additive) {
    out_pose.reset(skeleton_pose::type::additive);
  }
  else {
    out_pose.reset(skeleton_pose::type::absolute);
  }

  out_pose.sequence_start(_metadata.shallow_joint_index);

  acl_output_writer writer{.pose = &out_pose};
  _decompression_context.seek(time_s, acl::sample_rounding_policy::none);
  _decompression_context.decompress_tracks(writer);
}
}  // namespace eely::internal