#include "eely_importer/importer.h"

#include <eely/clip/clip_uncooked.h>
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project_uncooked.h>
#include <eely/skeleton/skeleton_uncooked.h>

#include <fbxsdk.h>

#include <fmt/format.h>

#include <gsl/assert>

#include <array>
#include <filesystem>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace eely {
static FbxSystemUnit measurement_unit_to_fbx_system_unit(const measurement_unit unit)
{
  switch (unit) {
    case measurement_unit::centimeters: {
      return FbxSystemUnit::cm;
    }

    case measurement_unit::meters: {
      return FbxSystemUnit::m;
    }

    default: {
      EXPECTS(false);
      return FbxSystemUnit::m;
    }
  }
}

static FbxAxisSystem axis_system_to_fbx_axis_system(const axis_system axes)
{
  FbxAxisSystem result;

  switch (axes) {
    case axis_system::y_up_x_right_z_forward: {
      FbxAxisSystem::ParseAxisSystem("xyz", result);
    } break;

    default: {
      EXPECTS(false);
      FbxAxisSystem::ParseAxisSystem("xyz", result);
    }
  }

  return result;
}

transform evaluate_local_transform(FbxNode* fbx_node,
                                   const bool is_root,
                                   const FbxTime& time = FBXSDK_TIME_INFINITE)
{
  FbxAMatrix& fbx_local_transform{is_root ? fbx_node->EvaluateGlobalTransform(time)
                                          : fbx_node->EvaluateLocalTransform(time)};
  FbxVector4 fbx_local_translation{fbx_local_transform.GetT()};
  FbxQuaternion fbx_local_rotation{fbx_local_transform.GetQ()};
  FbxVector4 fbx_local_scale{fbx_local_transform.GetS()};

  float3 local_translation{static_cast<float>(fbx_local_translation[0]),
                           static_cast<float>(fbx_local_translation[1]),
                           static_cast<float>(fbx_local_translation[2])};
  quaternion local_rotation{
      static_cast<float>(fbx_local_rotation[0]), static_cast<float>(fbx_local_rotation[1]),
      static_cast<float>(fbx_local_rotation[2]), static_cast<float>(fbx_local_rotation[3])};
  float3 local_scale{static_cast<float>(fbx_local_scale[0]), static_cast<float>(fbx_local_scale[1]),
                     static_cast<float>(fbx_local_scale[2])};

  return transform{local_translation, local_rotation, local_scale};
}

importer::importer(project_uncooked& project, const std::filesystem::path& path)
    : _filename(path.stem().string()),
      _project(project),
      _fbx_manager{FbxManager::Create()},
      _fbx_scene{FbxScene::Create(_fbx_manager, "eely import scene")}
{
  // Import the scene

  FbxIOSettings* fbx_io_settings{FbxIOSettings::Create(_fbx_manager, IOSROOT)};
  _fbx_manager->SetIOSettings(fbx_io_settings);

  FbxImporter* fbx_importer{FbxImporter::Create(_fbx_manager, "")};
  const bool imported{fbx_importer->Initialize(path.c_str(), -1, _fbx_manager->GetIOSettings())};
  if (!imported) {
    const std::string message{fmt::format("Could not import specified FBX file ({}): {}",
                                          path.c_str(),
                                          fbx_importer->GetStatus().GetErrorString())};
    throw std::runtime_error{message};
  }

  fbx_importer->Import(_fbx_scene);
  fbx_importer->Destroy();

  FbxSystemUnit fbx_unit{measurement_unit_to_fbx_system_unit(_project.get_measurement_unit())};
  fbx_unit.ConvertScene(_fbx_scene);

  FbxAxisSystem fbx_axis_system{axis_system_to_fbx_axis_system(_project.get_axis_system())};
  fbx_axis_system.DeepConvertScene(_fbx_scene);

  // Collect data that can be imported

  collect_fbx_skeleton_roots_recursively(_fbx_scene->GetRootNode(), _fbx_skeletons);

  const int stacks_count{_fbx_scene->GetSrcObjectCount<FbxAnimStack>()};
  for (int stack_index{0}; stack_index < stacks_count; ++stack_index) {
    FbxAnimStack* stack{_fbx_scene->GetSrcObject<FbxAnimStack>(stack_index)};

    const int layers_count{stack->GetSrcObjectCount<FbxAnimLayer>()};
    for (int layer_index{0}; layer_index < layers_count; ++layer_index) {
      FbxAnimLayer* layer{stack->GetSrcObject<FbxAnimLayer>(layer_index)};
      _fbx_animations.emplace_back(stack, layer);
    }
  }
}

importer::~importer()
{
  _fbx_manager->Destroy();
}

const std::vector<FbxSkeleton*>& importer::get_skeletons()
{
  return _fbx_skeletons;
}

skeleton_uncooked& importer::import_skeleton(const gsl::index skeleton_index)
{
  const FbxSkeleton* fbx_skeleton{_fbx_skeletons[skeleton_index]};

  std::vector<FbxSkeleton*> fbx_joints;
  std::vector<std::optional<gsl::index>> parent_indices;
  collect_fbx_skeletons_recursively(fbx_skeleton->GetNode(), std::nullopt, fbx_joints,
                                    parent_indices);

  std::vector<skeleton_uncooked::joint> joints;

  const gsl::index joints_count{std::ssize(fbx_joints)};
  for (gsl::index i{0}; i < joints_count; ++i) {
    FbxNode* fbx_node{fbx_joints[i]->GetNode()};
    const bool is_root{!parent_indices[i].has_value()};

    transform local_transform{evaluate_local_transform(fbx_node, is_root)};
    joints.push_back({.id = fbx_node->GetName(),
                      .parent_index = parent_indices[i],
                      .rest_pose_transform = local_transform});
  }

  const string_id id{fbx_skeleton->GetNode()->GetName()};
  std::unique_ptr<skeleton_uncooked> result{std::make_unique<skeleton_uncooked>(id)};
  result->set_joints(joints);

  _project.set_resource(std::move(result));

  return *_project.get_resource<skeleton_uncooked>(id);
}

clip_uncooked& importer::import_clip(const gsl::index animation_index,
                                     const skeleton_uncooked& skeleton)
{
  const auto& [fbx_anim_stack, fbx_anim_layer] = _fbx_animations[animation_index];

  std::vector<clip_uncooked_track> tracks;
  collect_tracks_recursively(fbx_anim_layer, _fbx_scene->GetRootNode(), skeleton, tracks);

  const string_id id{_filename};
  std::unique_ptr<clip_uncooked> result{std::make_unique<clip_uncooked>(id)};
  result->set_target_skeleton_id(skeleton.get_id());
  result->set_tracks(tracks);

  _project.set_resource(std::move(result));

  return *_project.get_resource<clip_uncooked>(id);
}

FbxSkeleton* importer::get_skeleton_attribute(FbxNode* fbx_node)
{
  const int attributes_count{fbx_node->GetNodeAttributeCount()};

  for (int i{0}; i < attributes_count; ++i) {
    FbxNodeAttribute* fbx_attribute{fbx_node->GetNodeAttributeByIndex(i)};
    const FbxNodeAttribute::EType type{fbx_attribute->GetAttributeType()};
    if (type == FbxNodeAttribute::EType::eSkeleton) {
      return dynamic_cast<FbxSkeleton*>(fbx_attribute);
    }
  }

  return nullptr;
}

void importer::collect_fbx_skeleton_roots_recursively(FbxNode* fbx_node,
                                                      std::vector<FbxSkeleton*>& out_result)
{
  FbxSkeleton* fbx_skeleton{get_skeleton_attribute(fbx_node)};

  if (fbx_skeleton == nullptr) {
    const int children_count{fbx_node->GetChildCount()};
    for (int i{0}; i < children_count; ++i) {
      collect_fbx_skeleton_roots_recursively(fbx_node->GetChild(i), out_result);
    }
  }
  else {
    out_result.push_back(fbx_skeleton);
  }
}

void importer::collect_fbx_skeletons_recursively(
    FbxNode* fbx_node,
    std::optional<gsl::index> parent_index,
    std::vector<FbxSkeleton*>& out_fbx_skeletons,
    std::vector<std::optional<gsl::index>>& out_parent_indices)
{
  FbxSkeleton* fbx_skeleton{get_skeleton_attribute(fbx_node)};
  if (fbx_skeleton != nullptr) {
    out_fbx_skeletons.push_back(fbx_skeleton);
    out_parent_indices.push_back(parent_index);

    const std::optional<gsl::index> new_parent_index{
        out_fbx_skeletons.empty() ? std::nullopt
                                  : std::optional<gsl::index>{out_fbx_skeletons.size() - 1}};

    const int children_count{fbx_node->GetChildCount()};
    for (int i{0}; i < children_count; ++i) {
      collect_fbx_skeletons_recursively(fbx_node->GetChild(i), new_parent_index, out_fbx_skeletons,
                                        out_parent_indices);
    }
  }
}

void importer::collect_tracks_recursively(FbxAnimLayer* fbx_anim_layer,
                                          FbxNode* fbx_node,
                                          const skeleton_uncooked& skeleton,
                                          std::vector<clip_uncooked_track>& out_tracks)
{
  enum property { translation = 1 << 0, rotation = 1 << 1, scale = 1 << 2 };

  const std::vector<skeleton_uncooked::joint>& joints{skeleton.get_joints()};
  const auto joint_find_iter{std::find_if(joints.begin(), joints.end(), [fbx_node](const auto& j) {
    return j.id == fbx_node->GetName();
  })};

  const bool is_root =
      joint_find_iter != joints.end() && !joint_find_iter->parent_index.has_value();

  std::unordered_map<property, std::array<const FbxAnimCurve*, 3>> property_to_curves;

  property_to_curves[property::translation] = {
      fbx_node->LclTranslation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_X),
      fbx_node->LclTranslation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Y),
      fbx_node->LclTranslation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Z)};

  property_to_curves[property::rotation] = {
      fbx_node->LclRotation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_X),
      fbx_node->LclRotation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Y),
      fbx_node->LclRotation.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Z)};

  property_to_curves[property::scale] = {
      fbx_node->LclScaling.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_X),
      fbx_node->LclScaling.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Y),
      fbx_node->LclScaling.GetCurve(fbx_anim_layer, FBXSDK_CURVENODE_COMPONENT_Z)};

  std::map<FbxTime, property> time_to_properties;
  for (const auto& [prop, curves] : property_to_curves) {
    for (const FbxAnimCurve* curve : curves) {
      if (curve == nullptr) {
        continue;
      }

      const int keys_count{curve->KeyGetCount()};
      for (int i{0}; i < keys_count; ++i) {
        const FbxTime fbx_time{curve->KeyGet(i).GetTime()};
        property& props{time_to_properties[fbx_time]};
        props = static_cast<property>(props | prop);
      }
    }
  }

  if (!time_to_properties.empty()) {
    const FbxTime& first_fbx_time{time_to_properties.begin()->first};
    const float start_time_s{static_cast<float>(first_fbx_time.GetSecondDouble())};

    clip_uncooked_track track;
    track.joint_id = fbx_node->GetName();

    for (const auto& [fbx_time, props] : time_to_properties) {
      const float time_s{static_cast<float>(fbx_time.GetSecondDouble()) - start_time_s};

      transform local_transform{evaluate_local_transform(fbx_node, is_root, fbx_time)};

      if ((props & property::translation) != 0) {
        track.keys[time_s].translation = local_transform.translation;
      }

      if ((props & property::rotation) != 0) {
        track.keys[time_s].rotation = local_transform.rotation;
      }

      if ((props & property::scale) != 0) {
        track.keys[time_s].scale = local_transform.scale;
      }
    }

    out_tracks.push_back(std::move(track));
  }

  const int children_count{fbx_node->GetChildCount()};
  for (int i{0}; i < children_count; ++i) {
    collect_tracks_recursively(fbx_anim_layer, fbx_node->GetChild(i), skeleton, out_tracks);
  }
}
}  // namespace eely