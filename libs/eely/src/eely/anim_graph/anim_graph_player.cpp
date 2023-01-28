#include "eely/anim_graph/anim_graph_player.h"

#include "eely/anim_graph/anim_graph.h"
#include "eely/anim_graph/anim_graph_node_and.h"
#include "eely/anim_graph/anim_graph_node_base.h"
#include "eely/anim_graph/anim_graph_node_blend.h"
#include "eely/anim_graph/anim_graph_node_clip.h"
#include "eely/anim_graph/anim_graph_node_param.h"
#include "eely/anim_graph/anim_graph_node_param_comparison.h"
#include "eely/anim_graph/anim_graph_node_random.h"
#include "eely/anim_graph/anim_graph_node_state.h"
#include "eely/anim_graph/anim_graph_node_state_condition.h"
#include "eely/anim_graph/anim_graph_node_state_machine.h"
#include "eely/anim_graph/anim_graph_node_state_transition.h"
#include "eely/anim_graph/anim_graph_node_sum.h"
#include "eely/anim_graph/anim_graph_player_context.h"
#include "eely/anim_graph/anim_graph_player_node_and.h"
#include "eely/anim_graph/anim_graph_player_node_base.h"
#include "eely/anim_graph/anim_graph_player_node_blend.h"
#include "eely/anim_graph/anim_graph_player_node_clip.h"
#include "eely/anim_graph/anim_graph_player_node_param.h"
#include "eely/anim_graph/anim_graph_player_node_param_comparison.h"
#include "eely/anim_graph/anim_graph_player_node_pose_base.h"
#include "eely/anim_graph/anim_graph_player_node_random.h"
#include "eely/anim_graph/anim_graph_player_node_speed.h"
#include "eely/anim_graph/anim_graph_player_node_state.h"
#include "eely/anim_graph/anim_graph_player_node_state_condition.h"
#include "eely/anim_graph/anim_graph_player_node_state_machine.h"
#include "eely/anim_graph/anim_graph_player_node_state_transition.h"
#include "eely/anim_graph/anim_graph_player_node_sum.h"
#include "eely/base/base_utils.h"
#include "eely/base/graph.h"
#include "eely/clip/clip.h"
#include "eely/job/job_queue.h"
#include "eely/params/params.h"
#include "eely/project/project.h"
#include "eely/skeleton/skeleton_pose.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace eely {
anim_graph_player::anim_graph_player(const anim_graph& anim_graph)
    : _project{anim_graph.get_project()},
      _job_queue{*_project.get_resource<skeleton>(anim_graph.get_skeleton_id())}
{
  using namespace eely::internal;

  // Runtime nodes are created in three steps:
  //  - constructing the nodes
  //  - filling their data (done after creation due to possible circular dependencies)
  //  - initializing state's breakpoints (they require fully inited states and transitions)

  const std::vector<anim_graph_node_uptr>& nodes{anim_graph.get_nodes()};

  std::unordered_map<int, anim_graph_player_node_base*> id_to_player_node;

  // Create nodes

  const int root_node_id{anim_graph.get_root_node_id()};

  for (const anim_graph_node_uptr& node : nodes) {
    const int node_id{node->get_id()};

    EXPECTS(!id_to_player_node.contains(node_id));

    anim_graph_player_node_uptr player_node{create_player_node(node)};
    EXPECTS(player_node);

    id_to_player_node[node_id] = player_node.get();
    if (node_id == root_node_id) {
      _root_node = player_node.get();
    }

    _nodes.push_back(std::move(player_node));
  }

  EXPECTS(!_nodes.empty());
  EXPECTS(_root_node != nullptr);

  // Fill node's data

  for (const anim_graph_node_uptr& node : nodes) {
    const int node_id{node->get_id()};

    EXPECTS(id_to_player_node.contains(node_id));

    anim_graph_player_node_base* player_node{id_to_player_node[node_id]};
    init_player_node(node, player_node, id_to_player_node);
  }

  // Initialize breakpoints

  for (const anim_graph_node_uptr& node : nodes) {
    if (node->get_type() != anim_graph_node_type::state) {
      continue;
    }

    const int node_id{node->get_id()};

    EXPECTS(id_to_player_node.contains(node_id));

    anim_graph_player_node_base* player_node{id_to_player_node[node_id]};
    auto* player_node_state{polymorphic_downcast<anim_graph_player_node_state*>(player_node)};

    player_node_state->update_breakpoints();
  }
}

void anim_graph_player::play(float dt_s, const params& params, skeleton_pose& out_pose)
{
  using namespace eely::internal;

  ++_play_counter;

  anim_graph_player_context context{
      .job_queue = _job_queue, .params = params, .play_counter = _play_counter, .dt_s = dt_s};

  _root_node->compute(context);

  _job_queue.execute(out_pose);
}

internal::anim_graph_player_node_uptr anim_graph_player::create_player_node(
    const anim_graph_node_uptr& node)
{
  using namespace eely::internal;

  switch (node->get_type()) {
    case anim_graph_node_type::and_logic: {
      return std::make_unique<anim_graph_player_node_and>();
    } break;

    case anim_graph_node_type::blend: {
      return std::make_unique<anim_graph_player_node_blend>();
    } break;

    case anim_graph_node_type::clip: {
      const auto* node_clip = polymorphic_downcast<const anim_graph_node_clip*>(node.get());
      const auto& clip{*_project.get_resource<eely::clip>(node_clip->get_clip_id())};
      return std::make_unique<anim_graph_player_node_clip>(clip);
    } break;

    case anim_graph_node_type::param_comparison: {
      const auto* node_param_comparison =
          polymorphic_downcast<const anim_graph_node_param_comparison*>(node.get());
      return std::make_unique<anim_graph_player_node_param_comparison>(
          node_param_comparison->get_param_id(), node_param_comparison->get_value(),
          node_param_comparison->get_op());
    } break;

    case anim_graph_node_type::param: {
      const auto* node_param = polymorphic_downcast<const anim_graph_node_param*>(node.get());
      return std::make_unique<anim_graph_player_node_param>(node_param->get_param_id());
    } break;

    case anim_graph_node_type::random: {
      return std::make_unique<anim_graph_player_node_random>();
    } break;

    case anim_graph_node_type::speed: {
      return std::make_unique<anim_graph_player_node_speed>();
    } break;

    case anim_graph_node_type::state_condition: {
      const auto* node_state_condition =
          polymorphic_downcast<const anim_graph_node_state_condition*>(node.get());
      return std::make_unique<anim_graph_player_node_state_condition>(
          node_state_condition->get_phase());
    } break;

    case anim_graph_node_type::state_machine: {
      return std::make_unique<anim_graph_player_node_state_machine>();
    } break;

    case anim_graph_node_type::state_transition: {
      const auto* node_state_transition =
          polymorphic_downcast<const anim_graph_node_state_transition*>(node.get());
      return std::make_unique<anim_graph_player_node_state_transition>(
          node_state_transition->get_transition_type(), node_state_transition->get_duration_s());
    } break;

    case anim_graph_node_type::state: {
      return std::make_unique<anim_graph_player_node_state>();
    } break;

    case anim_graph_node_type::sum: {
      return std::make_unique<anim_graph_player_node_sum>();
    } break;
  }
}

void anim_graph_player::init_player_node(
    const anim_graph_node_uptr& node,
    internal::anim_graph_player_node_base* player_node,
    const std::unordered_map<int, internal::anim_graph_player_node_base*>& id_to_player_node)
{
  using namespace eely::internal;

  switch (node->get_type()) {
    case anim_graph_node_type::and_logic: {
      const auto* node_and{polymorphic_downcast<const anim_graph_node_and*>(node.get())};
      auto* player_node_and{polymorphic_downcast<anim_graph_player_node_and*>(player_node)};

      // Init children nodes

      const std::vector<int>& children_nodes{node_and->get_children_nodes()};
      std::vector<anim_graph_player_node_base*>& player_children_nodes{
          player_node_and->get_children_nodes()};

      for (const int children_node_id : children_nodes) {
        EXPECTS(id_to_player_node.contains(children_node_id));
        auto* player_children_node{id_to_player_node.at(children_node_id)};

        player_children_nodes.push_back(player_children_node);
      }

    } break;

    case anim_graph_node_type::blend: {
      const auto* node_blend{polymorphic_downcast<const anim_graph_node_blend*>(node.get())};
      auto* player_node_blend{polymorphic_downcast<anim_graph_player_node_blend*>(player_node)};

      // Init children pose nodes

      const auto& pose_nodes_data{node_blend->get_pose_nodes()};
      std::vector<anim_graph_player_node_blend::pose_node_data>& player_pose_nodes_data{
          player_node_blend->get_pose_nodes()};

      for (const anim_graph_node_blend::pose_node_data& pose_node_data : pose_nodes_data) {
        EXPECTS(pose_node_data.id.has_value());
        const int pose_node_id{pose_node_data.id.value()};

        EXPECTS(id_to_player_node.contains(pose_node_id));
        auto* player_pose_node{id_to_player_node.at(pose_node_id)};

        player_pose_nodes_data.push_back(
            {.node = *polymorphic_downcast<anim_graph_player_node_pose_base*>(player_pose_node),
             .factor = pose_node_data.factor});
      }

      // Init factor node

      EXPECTS(node_blend->get_factor_node_id().has_value());
      const int factor_node_id{node_blend->get_factor_node_id().value()};

      EXPECTS(id_to_player_node.contains(factor_node_id));
      anim_graph_player_node_base* player_factor_node{id_to_player_node.at(factor_node_id)};

      player_node_blend->set_factor_node(player_factor_node);
    } break;

    case anim_graph_node_type::random: {
      const auto* node_random{polymorphic_downcast<const anim_graph_node_random*>(node.get())};
      auto* player_node_random{polymorphic_downcast<anim_graph_player_node_random*>(player_node)};

      // Init children nodes

      const auto& children_nodes{node_random->get_children_nodes()};
      std::vector<anim_graph_player_node_pose_base*> player_children_nodes;

      for (const int children_node_id : children_nodes) {
        EXPECTS(id_to_player_node.contains(children_node_id));
        auto* player_children_node{polymorphic_downcast<anim_graph_player_node_pose_base*>(
            id_to_player_node.at(children_node_id))};

        player_children_nodes.push_back(player_children_node);
      }

      player_node_random->set_children_nodes(std::move(player_children_nodes));

    } break;

    case anim_graph_node_type::speed: {
      const auto* node_speed{polymorphic_downcast<const anim_graph_node_speed*>(node.get())};
      auto* player_node_speed{polymorphic_downcast<anim_graph_player_node_speed*>(player_node)};

      EXPECTS(node_speed->get_child_node().has_value());
      const int child_node_id{node_speed->get_child_node().value()};

      EXPECTS(id_to_player_node.contains(child_node_id));
      auto* player_child_node{id_to_player_node.at(child_node_id)};

      EXPECTS(node_speed->get_speed_provider_node().has_value());
      const int speed_provider_node_id{node_speed->get_speed_provider_node().value()};

      EXPECTS(id_to_player_node.contains(speed_provider_node_id));
      auto* speed_provider_node{id_to_player_node.at(speed_provider_node_id)};

      player_node_speed->set_child_node(player_child_node);
      player_node_speed->set_speed_provider_node(speed_provider_node);
    } break;

    case anim_graph_node_type::state_machine: {
      const auto* node_state_machine{
          polymorphic_downcast<const anim_graph_node_state_machine*>(node.get())};
      auto* player_node_state_machine{
          polymorphic_downcast<anim_graph_player_node_state_machine*>(player_node)};

      const auto& state_nodes{node_state_machine->get_state_nodes()};
      std::vector<anim_graph_player_node_state*> player_state_nodes;

      for (const int state_node_id : state_nodes) {
        EXPECTS(id_to_player_node.contains(state_node_id));
        auto* player_node_state{polymorphic_downcast<anim_graph_player_node_state*>(
            id_to_player_node.at(state_node_id))};

        player_state_nodes.push_back(player_node_state);
      }

      player_node_state_machine->set_state_nodes(std::move(player_state_nodes));
    } break;

    case anim_graph_node_type::state_transition: {
      const auto* node_state_transition{
          polymorphic_downcast<const anim_graph_node_state_transition*>(node.get())};
      auto* player_node_state_transition{
          polymorphic_downcast<anim_graph_player_node_state_transition*>(player_node)};

      // Init condition node

      EXPECTS(node_state_transition->get_condition_node().has_value());
      const int condition_node_id{node_state_transition->get_condition_node().value()};

      EXPECTS(id_to_player_node.contains(condition_node_id));
      anim_graph_player_node_base* player_node_condition{id_to_player_node.at(condition_node_id)};

      player_node_state_transition->set_condition_node(player_node_condition);

      // Init destination state node

      EXPECTS(node_state_transition->get_destination_state_node().has_value());
      const int destination_state_node_id{
          node_state_transition->get_destination_state_node().value()};

      EXPECTS(id_to_player_node.contains(destination_state_node_id));
      anim_graph_player_node_state* player_node_destination_state{
          polymorphic_downcast<anim_graph_player_node_state*>(
              id_to_player_node.at(destination_state_node_id))};

      player_node_state_transition->set_original_destination_state_node(
          player_node_destination_state);
    } break;

    case anim_graph_node_type::state: {
      const auto* node_state{polymorphic_downcast<const anim_graph_node_state*>(node.get())};
      auto* player_node_state{polymorphic_downcast<anim_graph_player_node_state*>(player_node)};

      // Init out transition nodes

      const auto& out_transition_nodes{node_state->get_out_transition_nodes()};
      std::vector<anim_graph_player_node_state_transition*> out_transition_player_nodes;

      for (const int transition_node_id : out_transition_nodes) {
        EXPECTS(id_to_player_node.contains(transition_node_id));

        auto* player_node_transition{polymorphic_downcast<anim_graph_player_node_state_transition*>(
            id_to_player_node.at(transition_node_id))};
        out_transition_player_nodes.push_back(player_node_transition);
      }

      player_node_state->set_out_transitions(out_transition_player_nodes);

      // Init pose node

      EXPECTS(node_state->get_pose_node().has_value());
      const int pose_node_id{node_state->get_pose_node().value()};

      EXPECTS(id_to_player_node.contains(pose_node_id));
      auto* player_node_pose{polymorphic_downcast<anim_graph_player_node_pose_base*>(
          id_to_player_node.at(pose_node_id))};

      player_node_state->set_pose_node(player_node_pose);
    } break;

    case anim_graph_node_type::sum: {
      const auto* node_sum{polymorphic_downcast<const anim_graph_node_sum*>(node.get())};
      auto* player_node_sum{polymorphic_downcast<anim_graph_player_node_sum*>(player_node)};

      EXPECTS(node_sum->get_first_node_id().has_value());
      const int first_node_id{node_sum->get_first_node_id().value()};

      EXPECTS(id_to_player_node.contains(first_node_id));
      auto* player_node_first{polymorphic_downcast<anim_graph_player_node_pose_base*>(
          id_to_player_node.at(first_node_id))};

      EXPECTS(node_sum->get_second_node_id().has_value());
      const int second_node_id{node_sum->get_second_node_id().value()};

      EXPECTS(id_to_player_node.contains(second_node_id));
      auto* player_node_second{polymorphic_downcast<anim_graph_player_node_pose_base*>(
          id_to_player_node.at(second_node_id))};

      player_node_sum->set_first_node(player_node_first);
      player_node_sum->set_second_node(player_node_second);
    } break;

    default: {
      // Other types do not need any setup after creation
    } break;
  }
}
}  // namespace eely