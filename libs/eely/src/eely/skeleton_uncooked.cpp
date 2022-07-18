#include "eely/skeleton_uncooked.h"

#include "eely/resource_uncooked.h"
#include "eely/string_id.h"
#include "eely/transform.h"

#include <gsl/narrow>
#include <gsl/util>

#include <bit>
#include <optional>
#include <vector>

namespace eely {
static constexpr gsl::index bits_joints_count{11};

skeleton_uncooked::skeleton_uncooked(const string_id& id) : resource_uncooked(id) {}

skeleton_uncooked::skeleton_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  const gsl::index joints_count{reader.read(bits_joints_count)};

  _joints.resize(joints_count);
  for (gsl::index i{0}; i < joints_count; ++i) {
    joint& joint{_joints[i]};

    joint.id = string_id_deserialize(reader);

    const uint32_t has_parent{reader.read(1)};
    if (has_parent == 1) {
      joint.parent_index = reader.read(bits_joints_count);
    }

    joint.rest_pose_transform_joint_space = transform_deserialize(reader);
  }
}

void skeleton_uncooked::serialize(bit_writer& writer) const
{
  resource_uncooked::serialize(writer);

  const gsl::index joints_count{gsl::narrow<gsl::index>(_joints.size())};

  Expects(std::bit_width(gsl::narrow<size_t>(joints_count)) <= bits_joints_count);
  writer.write({.value = gsl::narrow_cast<uint32_t>(joints_count), .size_bits = bits_joints_count});

  for (const joint& joint : _joints) {
    string_id_serialize(joint.id, writer);

    if (joint.parent_index.has_value()) {
      writer.write({.value = 1, .size_bits = 1});
      writer.write({.value = gsl::narrow<uint32_t>(joint.parent_index.value()),
                    .size_bits = bits_joints_count});
    }
    else {
      writer.write({.value = 0, .size_bits = 1});
    }

    transform_serialize(joint.rest_pose_transform_joint_space, writer);
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
}  // namespace eely