#include "eely/project/resource_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace eely {
static constexpr gsl::index bits_resource_type{4};

enum class resource_uncooked_type { skeleton, clip, clip_additive, skeleton_mask };

void resource_uncooked::collect_dependencies(std::unordered_set<string_id>& out_dependencies) const
{
}

void resource_uncooked_serialize(const resource_uncooked& resource, bit_writer& writer)
{
  resource_uncooked_type type;
  if (dynamic_cast<const skeleton_uncooked*>(&resource) != nullptr) {
    type = resource_uncooked_type::skeleton;
  }
  else if (dynamic_cast<const clip_uncooked*>(&resource) != nullptr) {
    type = resource_uncooked_type::clip;
  }
  else if (dynamic_cast<const clip_additive_uncooked*>(&resource) != nullptr) {
    type = resource_uncooked_type::clip_additive;
  }
  else if (dynamic_cast<const skeleton_mask_uncooked*>(&resource) != nullptr) {
    type = resource_uncooked_type::skeleton_mask;
  }
  else {
    throw std::runtime_error("Unknown resource type for serialization");
  }

  writer.write({.value = static_cast<uint32_t>(type), .size_bits = bits_resource_type});

  resource.serialize(writer);
}

std::unique_ptr<resource_uncooked> resource_uncooked_deserialize(bit_reader& reader)
{
  const auto type{static_cast<resource_uncooked_type>(reader.read(bits_resource_type))};

  resource_base* resource_ptr{nullptr};
  switch (type) {
    case resource_uncooked_type::skeleton: {
      return std::make_unique<skeleton_uncooked>(reader);
    } break;

    case resource_uncooked_type::clip: {
      return std::make_unique<clip_uncooked>(reader);
    } break;

    case resource_uncooked_type::clip_additive: {
      return std::make_unique<clip_additive_uncooked>(reader);
    } break;

    case resource_uncooked_type::skeleton_mask: {
      return std::make_unique<skeleton_mask_uncooked>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown resource type for deserialization");
    } break;
  }
}
}  // namespace eely