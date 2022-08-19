#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <unordered_map>
#include <unordered_set>

namespace eely {
skeleton_mask_uncooked::skeleton_mask_uncooked(bit_reader& reader) : resource_uncooked(reader)
{
  using namespace eely::internal;

  _skeleton_id = string_id_deserialize(reader);

  gsl::index weights_count{reader.read(bits_joints_count)};
  for (gsl::index i{0}; i < weights_count; ++i) {
    string_id id{string_id_deserialize(reader)};
    float weight{bit_cast<float>(reader.read(32))};
    _weights.insert({std::move(id), weight});
  }
}

skeleton_mask_uncooked::skeleton_mask_uncooked(const string_id& id) : resource_uncooked(id) {}

void skeleton_mask_uncooked::serialize(bit_writer& writer) const
{
  using namespace eely::internal;

  string_id_serialize(_skeleton_id, writer);

  writer.write({.value = gsl::narrow<uint32_t>(_weights.size()), .size_bits = bits_joints_count});
  for (const auto& [joint_id, weight] : _weights) {
    string_id_serialize(joint_id, writer);
    writer.write({.value = bit_cast<uint32_t>(weight), .size_bits = 32});
  }
}

void skeleton_mask_uncooked::collect_dependencies(
    std::unordered_set<string_id>& out_dependencies) const
{
  out_dependencies.insert(_skeleton_id);
}

const string_id& skeleton_mask_uncooked::get_target_skeleton_id() const
{
  return _skeleton_id;
}

void skeleton_mask_uncooked::set_target_skeleton_id(string_id skeleton_id)
{
  _skeleton_id = std::move(skeleton_id);
}

const std::unordered_map<string_id, float>& skeleton_mask_uncooked::get_weights() const
{
  return _weights;
}

std::unordered_map<string_id, float>& skeleton_mask_uncooked::get_weights()
{
  return _weights;
}
}  // namespace eely