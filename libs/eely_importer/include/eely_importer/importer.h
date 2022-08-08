#pragma once

#include "eely/clip/clip_uncooked.h"
#include "eely/skeleton/skeleton_uncooked.h"
#include <eely/project/axis_system.h>
#include <eely/project/measurement_unit.h>
#include <eely/project/project_uncooked.h>

#include <fbxsdk.h>

#include <gsl/pointers>

#include <filesystem>
#include <vector>

namespace eely {
// Importer is responsible for importing uncooked resources from FBX files.
class importer final {
public:
  // Create importer for specified FBX file and project settings.
  explicit importer(project_uncooked& project, const std::filesystem::path& path);

  importer(const importer&) = delete;
  importer(importer&&) = delete;

  ~importer();

  importer& operator=(const importer&) = delete;
  importer& operator=(importer&&) = delete;

  // Get list of skeletons that can be imported from the provided file.
  [[nodiscard]] const std::vector<FbxSkeleton*>& get_skeletons();

  // Get list of animations that can be imported from the provided file.
  [[nodiscard]] const std::vector<std::pair<FbxAnimStack*, FbxAnimLayer*>>& get_animations();

  // Import skeleton with specified index.
  // Imported skeleton will be added to the project.
  skeleton_uncooked& import_skeleton(gsl::index skeleton_index);

  // Import specified clip.
  // Imported clip will be added to the project.
  clip_uncooked& import_clip(gsl::index animation_index, const skeleton_uncooked& skeleton);

private:
  static FbxSkeleton* get_skeleton_attribute(FbxNode* fbx_node);

  static void collect_fbx_skeleton_roots_recursively(FbxNode* fbx_node,
                                                     std::vector<FbxSkeleton*>& out_result);

  static void collect_fbx_skeletons_recursively(
      FbxNode* fbx_node,
      std::optional<gsl::index> parent_index,
      std::vector<FbxSkeleton*>& out_fbx_skeletons,
      std::vector<std::optional<gsl::index>>& out_parent_indices);

  static void collect_tracks_recursively(FbxAnimLayer* fbx_anim_layer,
                                         FbxNode* fbx_node,
                                         const skeleton_uncooked& skeleton,
                                         std::vector<clip_uncooked::track>& out_tracks);

  std::string _filename;
  project_uncooked& _project;
  gsl::owner<FbxManager*> _fbx_manager;
  FbxScene* _fbx_scene;

  std::vector<FbxSkeleton*> _fbx_skeletons;
  std::vector<std::pair<FbxAnimStack*, FbxAnimLayer*>> _fbx_animations;
};
}  // namespace eely