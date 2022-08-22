#include "eely/clip/clip.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_compression_scheme.h"
#include "eely/clip/clip_impl_acl.h"
#include "eely/clip/clip_impl_base.h"
#include "eely/clip/clip_impl_fixed.h"
#include "eely/clip/clip_impl_none.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/clip/clip_utils.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton.h"

#include <memory>

namespace eely {
clip::clip(const project& project, bit_reader& reader) : resource(project, reader)
{
  using namespace internal;

  const auto compression_scheme =
      static_cast<clip_compression_scheme>(reader.read(bits_clip_compression_scheme));

  switch (compression_scheme) {
    case clip_compression_scheme::none: {
      _impl = std::make_unique<clip_impl_none>(reader);
    } break;

    case clip_compression_scheme::fixed: {
      _impl = std::make_unique<clip_impl_fixed>(reader);
    } break;

    case clip_compression_scheme::acl: {
      _impl = std::make_unique<clip_impl_acl>(reader);
    } break;

    default: {
      EXPECTS(false);
    } break;
  }
}

clip::clip(const project& project, const clip_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  using namespace internal;

  const skeleton& skeleton{
      *project.get_resource<eely::skeleton>(uncooked.get_target_skeleton_id())};

  const float duration_s{uncooked.get_duration_s()};
  const std::vector<clip_uncooked_track>& tracks{uncooked.get_tracks()};

  switch (uncooked.get_compression_scheme()) {
    case clip_compression_scheme::none: {
      _impl = std::make_unique<clip_impl_none>(duration_s, tracks, false, skeleton);
    } break;

    case clip_compression_scheme::fixed: {
      _impl = std::make_unique<clip_impl_fixed>(duration_s, tracks, false, skeleton);
    } break;

    case clip_compression_scheme::acl: {
      _impl = std::make_unique<clip_impl_acl>(duration_s, tracks, false, skeleton);
    } break;

    default: {
      EXPECTS(false);
    } break;
  }
}

clip::clip(const project& project,
           const project_uncooked& project_uncooked,
           const clip_additive_uncooked& clip_uncooked)
    : resource(project, clip_uncooked.get_id())
{
  using namespace internal;

  const skeleton& skeleton{
      *project.get_resource<eely::skeleton>(clip_uncooked.get_target_skeleton_id())};

  float duration_s{0.0F};
  std::vector<clip_uncooked_track> tracks_additive;
  clip_calculate_additive_tracks(project_uncooked, clip_uncooked, duration_s, tracks_additive);

  switch (clip_uncooked.get_compression_scheme()) {
    case clip_compression_scheme::none: {
      _impl = std::make_unique<clip_impl_none>(duration_s, tracks_additive, true, skeleton);
    } break;

    case clip_compression_scheme::fixed: {
      _impl = std::make_unique<clip_impl_fixed>(duration_s, tracks_additive, true, skeleton);
    } break;

    case clip_compression_scheme::acl: {
      _impl = std::make_unique<clip_impl_acl>(duration_s, tracks_additive, true, skeleton);
    } break;

    default: {
      EXPECTS(false);
    } break;
  }
}

void clip::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  clip_compression_scheme compression_scheme;
  if (dynamic_cast<const clip_impl_none*>(_impl.get()) != nullptr) {
    compression_scheme = clip_compression_scheme::none;
  }
  else if (dynamic_cast<const clip_impl_fixed*>(_impl.get()) != nullptr) {
    compression_scheme = clip_compression_scheme::fixed;
  }
  else {
    compression_scheme = clip_compression_scheme::acl;
  }

  writer.write({.value = static_cast<uint32_t>(compression_scheme),
                .size_bits = bits_clip_compression_scheme});

  _impl->serialize(writer);
}

float clip::get_duration_s() const
{
  return _impl->get_metadata()->duration_s;
}

std::unique_ptr<clip_player_base> clip::create_player() const
{
  return _impl->create_player();
}
}  // namespace eely