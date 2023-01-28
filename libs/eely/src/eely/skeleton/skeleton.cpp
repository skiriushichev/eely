#include "eely/skeleton/skeleton.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/transform.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <limits>
#include <optional>
#include <vector>

namespace eely {
skeleton::skeleton(const project& project, const skeleton_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  using namespace eely::internal;

  const std::vector<skeleton_uncooked::joint>& joints{uncooked.get_joints()};

  const gsl::index joints_count{std::ssize(joints)};

  EXPECTS(joints_count > 0);
  EXPECTS(joints_count < joints_max_count);

  _joint_ids.resize(joints_count);
  _joint_parents.resize(joints_count);
  _rest_pose.resize(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_ids[i] = joints[i].id;

    const std::optional<gsl::index> parent_index{joints[i].parent_index};
    _joint_parents[i] = parent_index.value_or(null_index);

    _rest_pose[i] = joints[i].rest_pose_transform;
  }
}

skeleton::skeleton(const project& project, internal::bit_reader& reader) : resource(project, reader)
{
  using namespace eely::internal;

  const auto joints_count{bit_reader_read<gsl::index>(reader, bits_joints_count)};

  _joint_ids.resize(joints_count);
  _joint_parents.resize(joints_count);
  _rest_pose.resize(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_ids[i] = bit_reader_read<string_id>(reader);
  }

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_parents[i] = bit_reader_read<gsl::index>(reader, bits_joints_count);
  }

  for (gsl::index i{0}; i < joints_count; ++i) {
    _rest_pose[i] = bit_reader_read<transform>(reader);
  }
}

void skeleton::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  const gsl::index joints_count{std::ssize(_joint_ids)};
  bit_writer_write(writer, joints_count, bits_joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    bit_writer_write(writer, _joint_ids[i]);
  };

  for (gsl::index i{0}; i < joints_count; ++i) {
    bit_writer_write(writer, _joint_parents[i], bits_joints_count);
  };

  for (gsl::index i{0}; i < joints_count; ++i) {
    bit_writer_write(writer, _rest_pose[i]);
  }
}

gsl::index skeleton::get_joints_count() const
{
  return std::ssize(_joint_ids);
}

const string_id& skeleton::get_joint_id(gsl::index index) const
{
  return _joint_ids.at(index);
}

std::optional<gsl::index> skeleton::get_joint_index(const string_id& id) const
{
  const gsl::index joints_count{std::ssize(_joint_ids)};
  for (gsl::index i{0}; i < joints_count; ++i) {
    if (_joint_ids[i] == id) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<gsl::index> skeleton::get_joint_parent_index(gsl::index index) const
{
  gsl::index parent_index{_joint_parents.at(index)};
  return (parent_index == null_index) ? std::nullopt : std::optional<gsl::index>{parent_index};
}

const std::vector<transform>& skeleton::get_rest_pose_transforms() const
{
  return _rest_pose;
}
}  // namespace eely