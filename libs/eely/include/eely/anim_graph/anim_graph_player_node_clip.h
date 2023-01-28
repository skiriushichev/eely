#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/clip/clip.h"
#include "eely/clip/clip_player_base.h"
#include "eely/job/job_clip.h"

#include <any>
#include <memory>

namespace eely::internal {
// Runtime version of `anim_graph_node_clip`.
class anim_graph_player_node_clip final : public anim_graph_player_node_pose_base {
public:
  // Construct node with specified clip resource.
  explicit anim_graph_player_node_clip(const clip& clip);

protected:
  void compute_impl(const anim_graph_player_context& context, std::any& out_result) override;

private:
  std::unique_ptr<clip_player_base> _player;
  job_clip _job_clip;
};
}  // namespace eely::internal