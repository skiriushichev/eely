#include "eely/btree/btree_player_node_base.h"
#include "eely/job/job_add.h"

#include <gsl/util>

#include <vector>

namespace eely::internal {
// Node that adds two nodes together to produce additive animation.
class btree_player_node_add final : public btree_player_node_base {
public:
  explicit btree_player_node_add(btree_player_node_base& first, btree_player_node_base& second);

  void on_params_changed(const context& context) override;

  gsl::index play(const context& context) override;

private:
  btree_player_node_base& _first;
  btree_player_node_base& _second;
  internal::job_add _job;
};
}  // namespace eely::internal