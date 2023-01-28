#include "eely/skeleton_mask/skeleton_mask.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_utils.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <gsl/util>

#include <unordered_map>
#include <vector>

namespace eely {
skeleton_mask::skeleton_mask(const project& project, internal::bit_reader& reader)
    : resource(project, reader)
{
  using namespace eely::internal;

  const auto weights_count{bit_reader_read<gsl::index>(reader, internal::bits_joints_count)};

  _weights.resize(weights_count);
  for (gsl::index i{0}; i < weights_count; ++i) {
    _weights[i].translation = bit_reader_read<float>(reader);
    _weights[i].rotation = bit_reader_read<float>(reader);
    _weights[i].scale = bit_reader_read<float>(reader);
  }
}

skeleton_mask::skeleton_mask(const project& project, const skeleton_mask_uncooked& uncooked)
    : resource(project, uncooked.get_id())
{
  const skeleton& skeleton{
      *project.get_resource<eely::skeleton>(uncooked.get_target_skeleton_id())};

  const std::unordered_map<string_id, joint_weight>& weights_map{uncooked.get_weights()};

  const gsl::index joints_count{skeleton.get_joints_count()};
  _weights.resize(joints_count);
  for (gsl::index i{0}; i < joints_count; ++i) {
    auto find_iter{weights_map.find(skeleton.get_joint_id(i))};

    if (find_iter == weights_map.end()) {
      _weights[i] = {1.0F, 1.0F, 1.0F};
    }
    else {
      _weights[i] = find_iter->second;
    }
  }
}

void skeleton_mask::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource::serialize(writer);

  bit_writer_write(writer, _weights.size(), bits_joints_count);
  for (const joint_weight weight : _weights) {
    bit_writer_write(writer, weight.translation);
    bit_writer_write(writer, weight.rotation);
    bit_writer_write(writer, weight.scale);
  }
}
}  // namespace eely