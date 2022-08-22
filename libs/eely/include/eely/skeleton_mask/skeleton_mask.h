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
class skeleton_mask final : public resource {
public:
  explicit skeleton_mask(const project& project, bit_reader& reader);

  explicit skeleton_mask(const project& project, const skeleton_mask_uncooked& uncooked);

  void serialize(bit_writer& writer) const override;

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