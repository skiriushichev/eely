#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/btree/btree_node_add.h"
#include "eely/anim_graph/btree/btree_player_node_base.h"
#include "eely/job/job_add.h"

#include <gsl/util>

#include <optional>

namespace eely::internal {
class btree_player_node_add final : public btree_player_node_base {
public:
  explicit btree_player_node_add(btree_player_node_base& first_child,
                                 btree_player_node_base& second_child);

  void prepare(const anim_graph_context& context) override;

  gsl::index enqueue_job(const anim_graph_context& context) override;

private:
  btree_player_node_base& _first_child;
  btree_player_node_base& _second_child;
  internal::job_add _job;
};
}  // namespace eely::internal