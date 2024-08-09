#include "eely/project/project.h"

#include "eely/anim_graph/anim_graph.h"
#include "eely/anim_graph/anim_graph_uncooked.h"
#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
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
#include <span>
#include <vector>

namespace eely {
static constexpr gsl::index bits_resources_count{16};

project::project(const std::span<std::byte>& buffer)
{
  using namespace eely::internal;

  bit_reader reader{buffer};

  const auto resources_count{bit_reader_read<gsl::index>(reader, bits_resources_count)};

  for (gsl::index i{0}; i < resources_count; ++i) {
    std::unique_ptr<resource> r{resource_deserialize(*this, reader)};
    _resources[r->get_id()] = std::move(r);
  }
}

void project::cook(const project_uncooked& project_uncooked, const std::span<std::byte>& out_buffer)
{
  using namespace eely::internal;

  bit_writer writer{out_buffer};

  // Cook and write resources in topological order,
  // so that when a resource is being deserialized or cooked,
  // all of its dependencies are ready

  std::vector<const resource*> resources_ordered;

  project tmp_project;

  const auto cook_resource = [&tmp_project, &project_uncooked,
                              &resources_ordered](const resource_uncooked* resource_uncooked) {
    std::unique_ptr<resource> resource_cooked;

    if (const auto* skel_res_uncooked{dynamic_cast<const skeleton_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<skeleton>(tmp_project, *skel_res_uncooked);
    }
    else if (const auto* clip_res_uncooked{dynamic_cast<const clip_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<clip>(tmp_project, project_uncooked, *clip_res_uncooked);
    }
    else if (const auto* clip_additive_res_uncooked{dynamic_cast<const clip_additive_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<clip>(tmp_project, project_uncooked, *clip_additive_res_uncooked);
    }
    else if (const auto* skeleton_mask_res_uncooked{dynamic_cast<const skeleton_mask_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<skeleton_mask>(tmp_project, *skeleton_mask_res_uncooked);
    }
    else if (const auto* anim_graph_res_uncooked{dynamic_cast<const anim_graph_uncooked*>(resource_uncooked)}) {
      resource_cooked = std::make_unique<anim_graph>(tmp_project, *anim_graph_res_uncooked);
    }
    else {
      EXPECTS(false);
    }

    const string_id& id{resource_cooked->get_id()};

    tmp_project._resources[id] = std::move(resource_cooked);
    resources_ordered.push_back(tmp_project._resources[id].get());
  };

  project_uncooked.for_each_resource_topological(cook_resource);

  bit_writer_write(writer, resources_ordered.size(), bits_resources_count);

  for (const resource* r : resources_ordered) {
    resource_serialize(*r, writer);
  }
}
}  // namespace eely