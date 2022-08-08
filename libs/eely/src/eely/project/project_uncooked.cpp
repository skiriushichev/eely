#include "eely/project/project_uncooked.h"

#include "eely/base/assert.h"
#include "eely/project/axis_system.h"
#include "eely/project/measurement_unit.h"
#include "eely/project/resource_uncooked.h"

#include <bit>

namespace eely {
static constexpr gsl::index bits_resources_count{16};

project_uncooked::project_uncooked(bit_reader& reader)
{
  _measurement_unit = static_cast<measurement_unit>(reader.read(bits_measurement_units));
  _axis_system = static_cast<axis_system>(reader.read(bits_axis_system));

  const gsl::index resources_count{reader.read(bits_resources_count)};

  for (gsl::index i{0}; i < resources_count; ++i) {
    std::unique_ptr<resource_uncooked> r{resource_uncooked_deserialize(reader)};
    set_resource(std::move(r));
  }
}

project_uncooked::project_uncooked(measurement_unit measurement_unit, axis_system axis_system)
    : _measurement_unit{measurement_unit}, _axis_system{axis_system}
{
}

void project_uncooked::serialize(bit_writer& writer)
{
  writer.write(
      {.value = static_cast<uint32_t>(_measurement_unit), .size_bits = bits_measurement_units});

  writer.write({.value = static_cast<uint32_t>(_axis_system), .size_bits = bits_axis_system});

  writer.write(
      {.value = static_cast<uint32_t>(_resources.size()), .size_bits = bits_resources_count});

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