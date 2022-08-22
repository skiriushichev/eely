#include "eely/project/project.h"

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/btree/btree.h"
#include "eely/btree/btree_uncooked.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_uncooked.h"
#include "eely/project/project_uncooked.h"
#include "eely/project/resource.h"
#include "eely/project/resource_uncooked.h"
#include "eely/skeleton/skeleton.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include "eely/skeleton_mask/skeleton_mask.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <bit>
#include <memory>
#include <vector>

namespace eely {
static constexpr gsl::index bits_resources_count{16};

project::project(bit_reader& reader)
{
  const gsl::index resources_count{reader.read(bits_resources_count)};

  for (gsl::index i{0}; i < resources_count; ++i) {
    std::unique_ptr<resource> r{resource_deserialize(*this, reader)};
    _resources[r->get_id()] = std::move(r);
  }
}

void project::cook(const project_uncooked& project_uncooked, bit_writer& writer)
{
  // Cook and write resources in topological order,
  // so that when a resource is being deserialized or cooked,
  // all of its dependencies are ready

  std::vector<const resource*> resources_ordered;

  project tmp_project;

  const auto cook_resource = [&tmp_project, &project_uncooked,
                              &resources_ordered](const resource_uncooked* resource_uncooked) {
    std::unique_ptr<resource> resource_cooked;

    if (const auto* ru{dynamic_cast<const skeleton_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<skeleton>(tmp_project, *ru);
    }
    else if (const auto* ru{dynamic_cast<const clip_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<clip>(tmp_project, *ru);
    }
    else if (const auto* ru{dynamic_cast<const clip_additive_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<clip>(tmp_project, project_uncooked, *ru);
    }
    else if (const auto* ru{dynamic_cast<const skeleton_mask_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<skeleton_mask>(tmp_project, *ru);
    }
    else if (const auto* ru{dynamic_cast<const btree_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<btree>(tmp_project, *ru);
    }
    else {
      EXPECTS(false);
    }

    const string_id& id{resource_cooked->get_id()};

    tmp_project._resources[id] = std::move(resource_cooked);
    resources_ordered.push_back(tmp_project._resources[id].get());
  };

  project_uncooked.for_each_resource_topological(cook_resource);

  writer.write({.value = static_cast<uint32_t>(resources_ordered.size()),
                .size_bits = bits_resources_count});

  for (const resource* r : resources_ordered) {
    resource_serialize(*r, writer);
  }
}
}  // namespace eely