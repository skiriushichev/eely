#include "eely/project/project_uncooked.h"

#include "eely/base/assert.h"
#include "eely/base/bit_writer.h"
#include "eely/project/axis_system.h"
#include "eely/project/measurement_unit.h"
#include "eely/project/resource_uncooked.h"

#include <bit>
#include <memory>

namespace eely {
static constexpr gsl::index bits_resources_count{16};

project_uncooked::project_uncooked(internal::bit_reader& reader)
{
  _measurement_unit = bit_reader_read<measurement_unit>(reader, bits_measurement_units);
  _axis_system = bit_reader_read<axis_system>(reader, bits_axis_system);

  const auto resources_count{bit_reader_read<gsl::index>(reader, bits_resources_count)};

  for (gsl::index i{0}; i < resources_count; ++i) {
    std::unique_ptr<resource_uncooked> r{resource_uncooked_deserialize(*this, reader)};
    _resources[r->get_id()] = std::move(r);
  }
}

project_uncooked::project_uncooked(measurement_unit measurement_unit, axis_system axis_system)
    : _measurement_unit{measurement_unit}, _axis_system{axis_system}
{
}

void project_uncooked::serialize(internal::bit_writer& writer)
{
  using namespace eely::internal;

  bit_writer_write(writer, _measurement_unit, bits_measurement_units);
  bit_writer_write(writer, _axis_system, bits_axis_system);

  bit_writer_write(writer, _resources.size(), bits_resources_count);

  // Write resources in topological order,
  // so that when a resource is being deserialized,
  // all of its dependencies are already loaded
  for_each_resource_topological(
      [&writer](const resource_uncooked* r) { resource_uncooked_serialize(*r, writer); });
}

measurement_unit project_uncooked::get_measurement_unit() const
{
  return _measurement_unit;
}

axis_system project_uncooked::get_axis_system() const
{
  return _axis_system;
}
}  // namespace eely