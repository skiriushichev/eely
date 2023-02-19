#include "eely/skeleton/skeleton_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
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
  }
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
  }
}

const std::vector<skeleton_uncooked::joint>& skeleton_uncooked::get_joints() const
{
  return _joints;
}

void skeleton_uncooked::set_joints(std::vector<skeleton_uncooked::joint> joints)
{
  _joints = std::move(joints);
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
}  // namespace eely