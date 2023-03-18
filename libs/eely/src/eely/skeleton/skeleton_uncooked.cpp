#include "eely/skeleton/skeleton_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/math/quaternion.h"
#include "eely/math/transform.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <algorithm>
#include <bit>
#include <optional>
#include <vector>

namespace eely {
namespace internal {
template <>
inline skeleton_uncooked::constraint bit_reader_read<skeleton_uncooked::constraint>(
    bit_reader& reader)
{
  skeleton_uncooked::constraint constraint;
  constraint.parent_constraint_delta = bit_reader_read<quaternion>(reader);
  constraint.limit_twist_rad = bit_reader_read<std::optional<float>>(reader);
  constraint.limit_swing_y_rad = bit_reader_read<std::optional<float>>(reader);
  constraint.limit_swing_z_rad = bit_reader_read<std::optional<float>>(reader);

  return constraint;
}

void bit_writer_write(bit_writer& writer, const skeleton_uncooked::constraint& value)
{
  bit_writer_write(writer, value.parent_constraint_delta);
  bit_writer_write(writer, value.limit_twist_rad);
  bit_writer_write(writer, value.limit_swing_y_rad);
  bit_writer_write(writer, value.limit_swing_z_rad);
}

template <>
inline skeleton_uncooked::mapping bit_reader_read<skeleton_uncooked::mapping>(bit_reader& reader)
{
  skeleton_uncooked::mapping mapping;

  mapping.left_shoulder = bit_reader_read<string_id>(reader);
  mapping.left_arm = bit_reader_read<string_id>(reader);
  mapping.left_forearm = bit_reader_read<string_id>(reader);
  mapping.left_hand = bit_reader_read<string_id>(reader);

  mapping.right_shoulder = bit_reader_read<string_id>(reader);
  mapping.right_arm = bit_reader_read<string_id>(reader);
  mapping.right_forearm = bit_reader_read<string_id>(reader);
  mapping.right_hand = bit_reader_read<string_id>(reader);

  return mapping;
}

void bit_writer_write(bit_writer& writer, const skeleton_uncooked::mapping& value)
{
  bit_writer_write(writer, value.left_shoulder);
  bit_writer_write(writer, value.left_arm);
  bit_writer_write(writer, value.left_forearm);
  bit_writer_write(writer, value.left_hand);

  bit_writer_write(writer, value.right_shoulder);
  bit_writer_write(writer, value.right_arm);
  bit_writer_write(writer, value.right_forearm);
  bit_writer_write(writer, value.right_hand);
}
}  // namespace internal

skeleton_uncooked::skeleton_uncooked(const project_uncooked& project, internal::bit_reader& reader)
    : resource_uncooked{project, reader}
{
  using namespace eely::internal;

  const auto joints_count{bit_reader_read<gsl::index>(reader, bits_joints_count)};

  _joints.resize(joints_count);
  for (gsl::index i{0}; i < joints_count; ++i) {
    joint& joint{_joints[i]};
    joint.id = bit_reader_read<string_id>(reader);
    joint.parent_index = bit_reader_read<std::optional<gsl::index>>(reader, bits_joints_count);
    joint.rest_pose_transform = bit_reader_read<transform>(reader);
    joint.constraint = bit_reader_read<std::optional<constraint>>(reader);
  }

  _mapping = bit_reader_read<mapping>(reader);
}

skeleton_uncooked::skeleton_uncooked(const project_uncooked& project, string_id id)
    : resource_uncooked{project, std::move(id)}
{
}

void skeleton_uncooked::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  const gsl::index joints_count{std::ssize(_joints)};
  bit_writer_write(writer, joints_count, bits_joints_count);

  for (const joint& joint : _joints) {
    bit_writer_write(writer, joint.id);
    bit_writer_write(writer, joint.parent_index, bits_joints_count);
    bit_writer_write(writer, joint.rest_pose_transform);
    bit_writer_write(writer, joint.constraint);
  }

  bit_writer_write(writer, _mapping);
}

const std::vector<skeleton_uncooked::joint>& skeleton_uncooked::get_joints() const
{
  return _joints;
}

std::vector<skeleton_uncooked::joint>& skeleton_uncooked::get_joints()
{
  return _joints;
}

const skeleton_uncooked::joint* skeleton_uncooked::get_joint(const string_id& joint_id) const
{
  const auto iter{std::find_if(_joints.begin(), _joints.end(),
                               [&joint_id](const joint& j) { return j.id == joint_id; })};
  if (iter == _joints.end()) {
    return nullptr;
  }

  return &(*iter);
}

skeleton_uncooked::joint* skeleton_uncooked::get_joint(const string_id& joint_id)
{
  // To avoid code duplication.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return const_cast<skeleton_uncooked::joint*>(std::as_const(*this).get_joint(joint_id));
}

std::optional<gsl::index> skeleton_uncooked::get_joint_index(const string_id& joint_id) const
{
  const joint* joint{get_joint(joint_id)};
  if (joint != nullptr) {
    return std::distance(_joints.data(), joint);
  }

  return std::nullopt;
}

const skeleton_uncooked::mapping& skeleton_uncooked::get_mapping() const
{
  return _mapping;
}

skeleton_uncooked::mapping& skeleton_uncooked::get_mapping()
{
  return _mapping;
}
}  // namespace eely