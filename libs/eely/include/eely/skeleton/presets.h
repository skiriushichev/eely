#pragma once

#include "eely/skeleton/skeleton_uncooked.h"

namespace eely {
// Init skeleton imported from Mixamo service.
// This will setup skeleton mapping as well as humanoid joint constraints.
void mixamo_init(skeleton_uncooked& skeleton_uncooked);

// Init default joint limits for a humanoid skeleton.
// This only limits swing/twist limits and doesn't set constraint frames,
// because they depend on specific skeleton.
// Requires to have humanoid joints in the mapping.
void humanoid_constraints_limits_init(skeleton_uncooked& skeleton_uncooked);
}  // namespace eely