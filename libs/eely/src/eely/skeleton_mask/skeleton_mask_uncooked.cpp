#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include "eely/base/base_utils.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/base/string_id.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton_utils.h"

#include <gsl/narrow>
#include <gsl/util>

#include <unordered_map>
#include <unordered_set>

namespace eely {
skeleton_mask_uncooked::skeleton_mask_uncooked(const project_uncooked& project,
                                               internal::bit_reader& reader)
    : resource_uncooked{project, reader}
{
  using namespace eely::internal;

  _skeleton_id = bit_reader_read<string_id>(reader);

  const auto weights_count{bit_reader_read<gsl::index>(reader, bits_joints_count)};
  for (gsl::index i{0}; i < weights_count; ++i) {
    auto id{bit_reader_read<string_id>(reader)};
    const auto weight_translation{bit_reader_read<float>(reader)};
    const auto weight_rotation{bit_reader_read<float>(reader)};
    const auto weight_scale{bit_reader_read<float>(reader)};

    _weights.insert({id, {weight_translation, weight_rotation, weight_scale}});
  }
}

skeleton_mask_uncooked::skeleton_mask_uncooked(const project_uncooked& project, string_id id)
    : resource_uncooked{project, std::move(id)}
{
}

void skeleton_mask_uncooked::serialize(internal::bit_writer& writer) const
{
  using namespace eely::internal;

  resource_uncooked::serialize(writer);

  bit_writer_write(writer, _skeleton_id);

  bit_writer_write(writer, _weights.size(), bits_joints_count);
  for (const auto& [joint_id, weight] : _weights) {
    bit_writer_write(writer, joint_id);
    bit_writer_write(writer, weight.translation);
    bit_writer_write(writer, weight.rotation);
    bit_writer_write(writer, weight.scale);
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

const std::unordered_map<string_id, joint_weight>& skeleton_mask_uncooked::get_weights() const
{
  return _weights;
}

std::unordered_map<string_id, joint_weight>& skeleton_mask_uncooked::get_weights()
{
  return _weights;
}
}  // namespace eely