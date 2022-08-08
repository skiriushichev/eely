#include "eely/skeleton/skeleton.h"

#include "eely/base/assert.h"
#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/transform.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton_uncooked.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <limits>
#include <optional>
#include <vector>

namespace eely {
skeleton::skeleton(const skeleton_uncooked& uncooked) : resource(uncooked.get_id())
{
  using namespace eely::internal;

  const std::vector<skeleton_uncooked::joint>& joints{uncooked.get_joints()};

  const gsl::index joints_count{gsl::narrow<gsl::index>(joints.size())};

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

skeleton::skeleton(bit_reader& reader) : resource(reader)
{
  using namespace eely::internal;

  const gsl::index joints_count{reader.read(bits_joints_count)};

  _joint_ids.resize(joints_count);
  _joint_parents.resize(joints_count);
  _rest_pose.resize(joints_count);

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_ids[i] = string_id_deserialize(reader);
  }

  for (gsl::index i{0}; i < joints_count; ++i) {
    _joint_parents[i] = reader.read(bits_joints_count);
  }

  for (gsl::index i{0}; i < joints_count; ++i) {
    _rest_pose[i] = transform_deserialize(reader);
  }
}

void skeleton::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  resource_base::serialize(writer);

  const gsl::index joints_count{gsl::narrow<gsl::index>(_joint_ids.size())};

  writer.write({.value = gsl::narrow_cast<uint32_t>(joints_count), .size_bits = bits_joints_count});

  for (gsl::index i{0}; i < joints_count; ++i) {
    string_id_serialize(_joint_ids[i], writer);
  };

  for (gsl::index i{0}; i < joints_count; ++i) {
    writer.write(
        {.value = gsl::narrow_cast<uint32_t>(_joint_parents[i]), .size_bits = bits_joints_count});
  };

  for (gsl::index i{0}; i < joints_count; ++i) {
    transform_serialize(_rest_pose[i], writer);
  }
}

gsl::index skeleton::get_joints_count() const
{
  return gsl::narrow<gsl::index>(_joint_ids.size());
}

const string_id& skeleton::get_joint_id(gsl::index index) const
{
  return _joint_ids.at(index);
}

std::optional<gsl::index> skeleton::get_joint_index(const string_id& id) const
{
  for (gsl::index i{0}; i < _joint_ids.size(); ++i) {
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