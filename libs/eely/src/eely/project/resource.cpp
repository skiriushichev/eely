#include "eely/project/resource.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/clip/clip.h"
#include "eely/project/project.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton_mask/skeleton_mask.h"

#include <bit>
#include <stdexcept>

namespace eely {
static constexpr gsl::index bits_resource_type{4};

enum class resource_type { skeleton, clip, skeleton_mask };

void resource_serialize(const resource& resource, bit_writer& writer)
{
  resource_type type;
  if (dynamic_cast<const skeleton*>(&resource) != nullptr) {
    type = resource_type::skeleton;
  }
  else if (dynamic_cast<const clip*>(&resource) != nullptr) {
    type = resource_type::clip;
  }
  else if (dynamic_cast<const skeleton_mask*>(&resource) != nullptr) {
    type = resource_type::skeleton_mask;
  }
  else {
    throw std::runtime_error("Unknown resource type for serialization");
  }

  writer.write({.value = static_cast<uint32_t>(type), .size_bits = bits_resource_type});

  resource.serialize(writer);
}

std::unique_ptr<resource> resource_deserialize(const project& /*project*/, bit_reader& reader)
{
  const auto type{static_cast<resource_type>(reader.read(bits_resource_type))};

  resource_base* resource_ptr{nullptr};
  switch (type) {
    case resource_type::skeleton: {
      return std::make_unique<skeleton>(reader);
    } break;

    case resource_type::clip: {
      return std::make_unique<clip>(reader);
    } break;

    case resource_type::skeleton_mask: {
      return std::make_unique<skeleton_mask>(reader);
    } break;

    default: {
      throw std::runtime_error("Unknown resource type for deserialization");
    } break;
  }
}
}  // namespace eely