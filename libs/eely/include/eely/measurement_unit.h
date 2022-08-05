#pragma once

#include <gsl/util>

namespace eely {
// Describes units of measurement used within a project.
enum class measurement_unit { meters, centimeters };

static constexpr gsl::index bits_measurement_units{2};
}  // namespace eely