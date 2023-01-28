#include "eely/project/resource.h"

#include "eely/anim_graph/anim_graph.h"
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
resource::resource(const project& project, internal::bit_reader& reader)
    : resource_base(reader), _project{project}
{
}

resource::resource(const project& project, string_id id)
    : resource_base(std::move(id)), _project{project}
{
}

const project& resource::get_project() const
{
  return _project;
}

namespace internal {

static constexpr gsl::index bits_resource_type{4};

enum class resource_type { skeleton, clip, skeleton_mask, anim_graph };

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
  else if (dynamic_cast<const anim_graph*>(&resource) != nullptr) {
    type = resource_type::anim_graph;
  }
  else {
    throw std::runtime_error("Unknown resource type for serialization");
  }

  bit_writer_write(writer, type, bits_resource_type);

  resource.serialize(writer);
}

std::unique_ptr<resource> resource_deserialize(const project& project, bit_reader& reader)
{
  const auto type{bit_reader_read<resource_type>(reader, bits_resource_type)};

  switch (type) {
    case resource_type::skeleton: {
      return std::make_unique<skeleton>(project, reader);
    } break;

    case resource_type::clip: {
      return std::make_unique<clip>(project, reader);
    } break;

    case resource_type::skeleton_mask: {
      return std::make_unique<skeleton_mask>(project, reader);
    } break;

    case resource_type::anim_graph: {
      return std::make_unique<anim_graph>(project, reader);
    } break;

    default: {
      throw std::runtime_error("Unknown resource type for deserialization");
    } break;
  }
}
}  // namespace internal
}  // namespace eely