#include "eely/clip/clip_impl_acl.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_impl_base.h"
#include "eely/clip/clip_player_acl.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/clip/clip_utils.h"
#include "eely/skeleton/skeleton_utils.h"

#include <acl/compression/compress.h>
#include <acl/core/ansi_allocator.h>
#include <acl/decompression/decompress.h>

#include <gsl/narrow>
#include <gsl/util>

#include <malloc.h>
#include <memory>
#include <span>
#include <vector>

namespace eely::internal {
std::unique_ptr<uint8_t, decltype(&aligned_free)> acl_allocate_compressed_tracks_storage(
    const size_t size)
{
  const size_t alignment{alignof(acl::compressed_tracks)};
  const size_t aligned_size{align_size_to({.alignment = alignment, .size = size})};

  return {static_cast<uint8_t*>(aligned_alloc(alignment, aligned_size)), aligned_free};
}

// ACL needs equal number of samples for every track in a clip.
// Uncooked data only contains keys that are authored,
// thus we need to sample the clip at required rate and pass results to ACL.

static constexpr gsl::index acl_sample_rate = 30;

// Compress uncooked clip.
std::unique_ptr<uint8_t, decltype(&std::free)> acl_compress(
    const float duration_s,
    const std::vector<clip_uncooked_track>& tracks,
    const skeleton& skeleton,
    acl::iallocator& acl_allocator)
{
  using namespace acl;

  struct sampled_track final {
    string_id joint_id;
    transform joint_rest_pose_transform;
    std::optional<gsl::index> joint_parent_index;

    std::vector<transform> samples;
  };

  const clip_sampling_info sampling_info{
      .time_from_s = 0.0F, .time_to_s = duration_s, .rate = acl_sample_rate};

  const gsl::index samples_count{clip_sampling_info_calculate_samples(sampling_info)};

  const gsl::index joints_count{skeleton.get_joints_count()};

  std::vector<sampled_track> sampled_tracks;
  sampled_tracks.reserve(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    const string_id& joint_id{skeleton.get_joint_id(i)};
    const std::optional<gsl::index> joint_parent_index{skeleton.get_joint_parent_index(i)};
    const transform& joint_rest_pose_transform{skeleton.get_rest_pose_transforms()[i]};

    auto track_iter{std::find_if(tracks.begin(), tracks.end(),
                                 [&joint_id](const auto& t) { return t.joint_id == joint_id; })};

    if (track_iter == tracks.end()) {
      // There is not track for this joint, but we still need to give ACL the data.
      // Populate it with rest pose transform.

      sampled_tracks.push_back(
          {.joint_id = joint_id,
           .joint_rest_pose_transform = joint_rest_pose_transform,
           .joint_parent_index = joint_parent_index,
           .samples = std::vector<transform>(samples_count, joint_rest_pose_transform)});
    }
    else {
      // There is a track, sample it

      std::vector<transform> samples;
      clip_sample_track(*track_iter, joint_rest_pose_transform, sampling_info, samples);

      sampled_tracks.push_back({.joint_id = joint_id,
                                .joint_rest_pose_transform = joint_rest_pose_transform,
                                .joint_parent_index = joint_parent_index,
                                .samples = std::move(samples)});
    }
  }

  // Populate ACL tracks
  // TODO: take `measurement_unit` into account here

  track_array_qvvf raw_track_list(acl_allocator, gsl::narrow<uint32_t>(joints_count));

  for (gsl::index track_index{0}; track_index < joints_count; ++track_index) {
    const sampled_track& track{sampled_tracks[track_index]};

    const transform& default_qvvf{track.joint_rest_pose_transform};

    rtm::qvvf acl_default_qvvf;
    acl_default_qvvf.rotation = rtm::quat_set(default_qvvf.rotation.x, default_qvvf.rotation.y,
                                              default_qvvf.rotation.z, default_qvvf.rotation.w);
    acl_default_qvvf.translation = rtm::vector_set(
        default_qvvf.translation.x, default_qvvf.translation.y, default_qvvf.translation.z);
    acl_default_qvvf.scale =
        rtm::vector_set(default_qvvf.scale.x, default_qvvf.scale.y, default_qvvf.scale.z);

    track_desc_transformf acl_track_description;
    acl_track_description.default_value = acl_default_qvvf;
    acl_track_description.output_index = gsl::narrow<uint32_t>(track_index);
    acl_track_description.parent_index =
        gsl::narrow<uint32_t>(track.joint_parent_index.value_or(k_invalid_track_index));
    acl_track_description.precision = 0.001F;
    acl_track_description.shell_distance = 0.3F;

    const gsl::index samples_size{std::ssize(track.samples)};

    track_qvvf acl_track{track_qvvf::make_reserve(acl_track_description, acl_allocator,
                                                  gsl::narrow<uint32_t>(samples_size),
                                                  acl_sample_rate)};

    for (gsl::index sample_index{0}; sample_index < samples_size; ++sample_index) {
      const transform& sample = track.samples[sample_index];

      rtm::qvvf acl_qvvf;
      acl_qvvf.rotation =
          rtm::quat_set(sample.rotation.x, sample.rotation.y, sample.rotation.z, sample.rotation.w);
      acl_qvvf.translation =
          rtm::vector_set(sample.translation.x, sample.translation.y, sample.translation.z);
      acl_qvvf.scale = rtm::vector_set(sample.scale.x, sample.scale.y, sample.scale.z);

      acl_track[gsl::narrow<uint32_t>(sample_index)] = acl_qvvf;
    }

    raw_track_list[gsl::narrow<uint32_t>(track_index)] = std::move(acl_track);
  }

  // Compress ACL tracks
  // TODO: output metrics

  compression_settings settings{get_default_compression_settings()};

  qvvf_transform_error_metric error_metric;
  settings.error_metric = &error_metric;

  output_stats stats;

  compressed_tracks* acl_compressed_tracks{nullptr};
  [[maybe_unused]] error_result error_result =
      compress_track_list(acl_allocator, raw_track_list, settings, acl_compressed_tracks, stats);
  EXPECTS(error_result.empty());

  auto storage{acl_allocate_compressed_tracks_storage(acl_compressed_tracks->get_size())};
  memcpy(storage.get(), acl_compressed_tracks, acl_compressed_tracks->get_size());

  acl_allocator.deallocate(acl_compressed_tracks, acl_compressed_tracks->get_size());

  return storage;
}

clip_impl_acl::clip_impl_acl(bit_reader& reader)
{
  // Metadata

  _metadata.duration_s = bit_reader_read<float>(reader);
  EXPECTS(_metadata.duration_s >= 0.0F);

  _metadata.is_additive = bit_reader_read<bool>(reader);

  _metadata.shallow_joint_index = bit_reader_read<gsl::index>(reader, bits_joints_count);

  // Data

  const gsl::index data_size{bit_reader_read<gsl::index>(reader, 32)};
  EXPECTS(data_size > 0);

  _acl_compressed_tracks_storage = acl_allocate_compressed_tracks_storage(data_size);

  std::span<uint8_t> data_span{_acl_compressed_tracks_storage.get(),
                               gsl::narrow<size_t>(data_size)};
  for (gsl::index i{0}; i < data_size; ++i) {
    data_span[i] = bit_reader_read<uint8_t>(reader);
  }

  acl::error_result error_result;
  _acl_compressed_tracks = acl::make_compressed_tracks(data_span.data(), &error_result);
  EXPECTS(error_result.empty());
  EXPECTS(_acl_compressed_tracks != nullptr);
}

clip_impl_acl::clip_impl_acl(const float duration_s,
                             const std::vector<clip_uncooked_track>& tracks,
                             const bool is_additive,
                             const skeleton& skeleton)
{
  _metadata.duration_s = duration_s;
  _metadata.is_additive = is_additive;
  _metadata.shallow_joint_index = std::numeric_limits<gsl::index>::max();
  for (const clip_uncooked_track& track : tracks) {
    const std::optional<gsl::index> joint_index_opt{skeleton.get_joint_index(track.joint_id)};
    if (!joint_index_opt.has_value()) {
      continue;
    }

    _metadata.shallow_joint_index =
        std::min(_metadata.shallow_joint_index, joint_index_opt.value());
  }

  _acl_compressed_tracks_storage = acl_compress(duration_s, tracks, skeleton, _acl_allocator);

  acl::error_result error_result;
  _acl_compressed_tracks =
      acl::make_compressed_tracks(_acl_compressed_tracks_storage.get(), &error_result);
  EXPECTS(error_result.empty());
  EXPECTS(_acl_compressed_tracks != nullptr);
}

void clip_impl_acl::serialize(bit_writer& writer) const
{
  // Metadata

  bit_writer_write(writer, _metadata.duration_s);
  bit_writer_write(writer, _metadata.is_additive);

  bit_writer_write(writer, _metadata.shallow_joint_index, bits_joints_count);

  // Data

  bit_writer_write(writer, _acl_compressed_tracks->get_size());

  std::span<uint8_t> data_span{_acl_compressed_tracks_storage.get(),
                               _acl_compressed_tracks->get_size()};
  for (gsl::index i{0}; i < _acl_compressed_tracks->get_size(); ++i) {
    bit_writer_write(writer, data_span[i]);
  }
}

const clip_metadata_base* clip_impl_acl::get_metadata() const
{
  return &_metadata;
}

std::unique_ptr<clip_player_base> clip_impl_acl::create_player() const
{
  return std::make_unique<clip_player_acl>(_metadata, *_acl_compressed_tracks);
}
}  // namespace eely::internal