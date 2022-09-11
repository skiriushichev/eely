#pragma once

#include "eely/base/assert.h"
#include "eely/base/bit_reader.h"
#include "eely/base/bit_writer.h"
#include "eely/project/project.h"
#include "eely/project/resource.h"
#include "eely/skeleton_mask/skeleton_mask_uncooked.h"

#include <gsl/util>

#include <vector>

namespace eely {
// Cooked version of a skeleton mask resource,
// which defines how different parts of a skeleton participate in certain operations.
class skeleton_mask final : public resource {
public:
  // Construct skeleton mask from a buffer.
  explicit skeleton_mask(const project& project, bit_reader& reader);

  // Construct skeleton mask from an uncooked counterpart.
  explicit skeleton_mask(const project& project, const skeleton_mask_uncooked& uncooked);

  void serialize(bit_writer& writer) const override;

  // Return weight of a joint.
  [[nodiscard]] float get_weight(gsl::index joint_index) const;

private:
  std::vector<float> _weights;
};

// Implementation

inline float skeleton_mask::get_weight(const gsl::index joint_index) const
{
  return _weights.at(joint_index);
}

}  // namespace eely