#include "eely/clip/clip_utils.h"

#include "eely/clip/clip_uncooked.h"
#include "eely/project/project_uncooked.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton_mask/skeleton_mask.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <optional>
#include <unordered_map>

namespace eely::internal {
gsl::index clip_sampling_info_calculate_samples(const clip_sampling_info& info)
{
  const float duration_s{info.time_to_s - info.time_from_s};
  return static_cast<gsl::index>(duration_s * static_cast<float>(info.rate)) + 1;
}

// Calculate transforms for a track's joint.
void clip_sample_track(const clip_uncooked_track& track,
                       const transform& default_value,
                       const clip_sampling_info& sampling_info,
                       std::vector<transform>& out_samples)
{
  const float sample_timestep_s{1.0F / static_cast<float>(sampling_info.rate)};
  const gsl::index samples_count{clip_sampling_info_calculate_samples(sampling_info)};

  for (gsl::index sample_index{0}; sample_index < samples_count; ++sample_index) {
    const float sample_time_s = static_cast<float>(sample_index) * sample_timestep_s;

    transform sample{clip_sample_component<transform_components::translation>(
                         track, default_value.translation, sample_time_s),
                     clip_sample_component<transform_components::rotation>(
                         track, default_value.rotation, sample_time_s),
                     clip_sample_component<transform_components::scale>(track, default_value.scale,
                                                                        sample_time_s)};

    out_samples.push_back(sample);
  }
}

void clip_calculate_tracks(const project_uncooked& project,
                           const clip_uncooked& uncooked,
                           std::vector<clip_uncooked_track>& out_tracks)
{
  const std::vector<clip_uncooked_track>& original_tracks{uncooked.get_tracks()};

  const skeleton_mask_uncooked* skeleton_mask{
      project.get_resource<skeleton_mask_uncooked>(uncooked.get_skeleton_mask_id())};
  if (skeleton_mask == nullptr) {
    // If there is no skeleton mask,
    // there is no alteration to the original tracks.
    out_tracks = original_tracks;
    return;
  }

  // Apply mask weights

  const std::unordered_map<string_id, joint_weight>& weights{skeleton_mask->get_weights()};

  for (const clip_uncooked_track& original_track : original_tracks) {
    clip_uncooked_track result_track{.joint_id = original_track.joint_id};

    for (const auto& [original_time, original_key] : original_track.keys) {
      clip_uncooked_key result_key;

      joint_weight weight{1.0F, 1.0F, 1.0F};

      auto weight_iter{weights.find(original_track.joint_id)};
      if (weight_iter != weights.end()) {
        weight = weight_iter->second;
      }

      if (!float_near(weight.translation, 0.0F) && original_key.translation.has_value()) {
        result_key.translation =
            float3_lerp(float3::zeroes, original_key.translation.value(), weight.translation);
      }

      if (!float_near(weight.rotation, 0.0F) && original_key.rotation.has_value()) {
        result_key.rotation =
            quaternion_slerp(quaternion::identity, original_key.rotation.value(), weight.rotation);
      }

      if (!float_near(weight.scale, 0.0F) && original_key.scale.has_value()) {
        result_key.scale = float3_lerp(float3::zeroes, original_key.scale.value(), weight.scale);
      }

      if (result_key.translation.has_value() || result_key.rotation.has_value() ||
          result_key.scale.has_value()) {
        result_track.keys[original_time] = result_key;
      }
    }

    if (!result_track.keys.empty()) {
      out_tracks.push_back(std::move(result_track));
    }
  }
}

void clip_calculate_additive_tracks(const project_uncooked& project,
                                    const clip_additive_uncooked& uncooked,
                                    float& out_duration,
                                    std::vector<clip_uncooked_track>& out_tracks)
{
  // Collect clips info

  const clip_uncooked& clip_base{*project.get_resource<clip_uncooked>(uncooked.get_base_clip_id())};
  const clip_uncooked& clip_source{
      *project.get_resource<clip_uncooked>(uncooked.get_source_clip_id())};

  const std::optional<clip_additive_uncooked::range>& clip_base_range_opt{
      uncooked.get_base_clip_range()};
  const std::optional<clip_additive_uncooked::range>& clip_source_range_opt{
      uncooked.get_source_clip_range()};

  clip_additive_uncooked::range clip_base_range;
  if (clip_base_range_opt.has_value()) {
    clip_base_range = clip_base_range_opt.value();
  }
  else {
    clip_base_range.from_s = 0.0F;
    clip_base_range.to_s = clip_base.get_duration_s();
  }

  clip_additive_uncooked::range clip_source_range;
  if (clip_source_range_opt.has_value()) {
    clip_source_range = clip_source_range_opt.value();
  }
  else {
    clip_source_range.from_s = 0.0F;
    clip_source_range.to_s = clip_base.get_duration_s();
  }

  out_duration = clip_source_range.to_s - clip_source_range.from_s;

  // Collect skeleton info

  const skeleton_uncooked& skeleton{
      *project.get_resource<skeleton_uncooked>(clip_source.get_target_skeleton_id())};

  const std::unordered_map<string_id, joint_weight>* joint_weights{nullptr};
  const skeleton_mask_uncooked* skeleton_mask{
      project.get_resource<skeleton_mask_uncooked>(uncooked.get_skeleton_mask_id())};
  if (skeleton_mask != nullptr) {
    joint_weights = &skeleton_mask->get_weights();
  }

  const auto get_joint_weight = [joint_weights](const string_id& joint_id) {
    joint_weight weights{1.0F};
    if (joint_weights != nullptr) {
      auto iter{joint_weights->find(joint_id)};
      if (iter != joint_weights->end()) {
        weights = iter->second;
      }
    }

    return weights;
  };

  // Prepare sampling parameters

  clip_sampling_info source_sampling_info{
      .time_from_s = clip_source_range.from_s, .time_to_s = clip_source_range.to_s, .rate = 30};
  const gsl::index source_samples{clip_sampling_info_calculate_samples(source_sampling_info)};

  clip_sampling_info base_sampling_info{.time_from_s = clip_base_range.from_s,
                                        .time_to_s = clip_base_range.to_s};

  base_sampling_info.rate = 30;
  if (!float_near(clip_base_range.from_s, clip_base_range.to_s)) {
    const float base_duration_s{clip_base_range.to_s - clip_base_range.from_s};
    base_sampling_info.rate = static_cast<gsl::index>(
        std::round(static_cast<float>(source_samples - 1) / base_duration_s));
  }

  // Sample source animation

  std::unordered_map<string_id, std::vector<transform>> source_sampled_tracks;

  for (const clip_uncooked_track& t : clip_source.get_tracks()) {
    const skeleton_uncooked::joint* joint{skeleton.get_joint(t.joint_id)};
    if (joint == nullptr) {
      continue;
    }

    const joint_weight weight{get_joint_weight(joint->id)};
    if (float_near(weight.translation, 0.0F) && float_near(weight.rotation, 0.0F) &&
        float_near(weight.scale, 0.0F)) {
      continue;
    }

    clip_sample_track(t, joint->rest_pose_transform, source_sampling_info,
                      source_sampled_tracks[t.joint_id]);
  }

  const float source_sample_timestep{1.0F / static_cast<float>(source_sampling_info.rate)};

  // Calculate difference between tracks

  for (const auto& kvp : source_sampled_tracks) {
    const string_id& joint_id{kvp.first};

    const joint_weight weight{get_joint_weight(joint_id)};

    clip_uncooked_track diff_track;
    diff_track.joint_id = joint_id;

    std::vector<transform> base_track_samples;

    const auto base_track_find_iter{
        std::find_if(clip_base.get_tracks().begin(), clip_base.get_tracks().end(),
                     [&joint_id](const auto& t) { return t.joint_id == joint_id; })};
    if (base_track_find_iter == clip_base.get_tracks().end()) {
      // No counterpart track
      // Calculate diff with rest pose transform
      base_track_samples.push_back(skeleton.get_joint(joint_id)->rest_pose_transform);
    }
    else {
      const clip_uncooked_track& base_track{*base_track_find_iter};
      clip_sample_track(base_track, skeleton.get_joint(joint_id)->rest_pose_transform,
                        base_sampling_info, base_track_samples);
    }

    // Sampling could give either 1 sample or the same number as source

    for (size_t i = 0; i < kvp.second.size(); ++i) {
      const float time_s{source_sampling_info.time_from_s +
                         source_sample_timestep * static_cast<float>(i)};

      // Use `std::min` when accesing base track to account for either 1 sample or N samples
      const transform delta{transform_diff(
          kvp.second[i], base_track_samples[std::min(i, base_track_samples.size() - 1)])};

      if (!float_near(weight.translation, 0.0F)) {
        diff_track.keys[time_s].translation =
            float3_lerp(float3::zeroes, delta.translation, weight.translation);
      }

      if (!float_near(weight.rotation, 0.0F)) {
        diff_track.keys[time_s].rotation =
            quaternion_slerp(quaternion::identity, delta.rotation, weight.rotation);
      }

      if (!float_near(weight.scale, 0.0F)) {
        diff_track.keys[time_s].scale = float3_lerp(float3::ones, delta.scale, weight.scale);
      }
    }

    out_tracks.push_back(diff_track);
  }
}
}  // namespace eely::internal