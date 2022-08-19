#include "eely/skeleton_mask/skeleton_mask.h"

#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <gsl/util>

#include <unordered_map>
#include <vector>

namespace eely {
skeleton_mask::skeleton_mask(bit_reader& reader) : resource(reader)
{
  gsl::index weights_count{reader.read(internal::bits_joints_count)};

  _weights.resize(weights_count);
  for (gsl::index i{0}; i < weights_count; ++i) {
    _weights[i] = bit_cast<float>(reader.read(32));
  }
}

skeleton_mask::skeleton_mask(const project& project, const skeleton_mask_uncooked& uncooked)
    : resource(uncooked.get_id())
{
  const skeleton& skeleton{
      *project.get_resource<eely::skeleton>(uncooked.get_target_skeleton_id())};

  const std::unordered_map<string_id, float>& weights_map{uncooked.get_weights()};

  const gsl::index joints_count{skeleton.get_joints_count()};
  _weights.resize(joints_count);
  for (gsl::index i{0}; i < joints_count; ++i) {
    auto find_iter{weights_map.find(skeleton.get_joint_id(i))};

    if (find_iter == weights_map.end()) {
      _weights[i] = 1.0F;
    }
    else {
      _weights[i] = find_iter->second;
    }
  }
}

void skeleton_mask::serialize(bit_writer& writer) const
{
  resource_base::serialize(writer);

  writer.write(
      {.value = gsl::narrow<uint32_t>(_weights.size()), .size_bits = internal::bits_joints_count});

  for (const float weight : _weights) {
    writer.write({.value = bit_cast<uint32_t>(weight), .size_bits = 32});
  }
}
}  // namespace eely