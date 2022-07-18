#pragma once

#include <memory>
#include <unordered_map>

namespace eely {
// Represents map of assets used in an application (materials, meshes, uniforms etc.).
// Term "asset" is used to distinguish them from `eely::resource`,
// and anything identifiable by a key and existing as a single instance can be treated as such.
//
// This map owns the assets, which means that they are never released
// after being used for the first time, which is fine for this project's purposes for now,
// since it simplifies code and structures.
template <typename TResKey, typename TRes>
class asset_map final {
public:
  // Get asset with specified key.
  const TRes& get(const TResKey& key);

  // Destroy all assets.
  void clear();

private:
  std::unordered_map<TResKey, std::unique_ptr<TRes>> _assets;
};

template <typename TResKey, typename TRes>
const TRes& asset_map<TResKey, TRes>::get(const TResKey& key)
{
  auto iter = _assets.find(key);
  if (iter != _assets.end()) {
    return *(iter->second.get());
  }

  // There was no resource with this key
  // Needs to be created
  std::unique_ptr<TRes> asset{std::make_unique<TRes>(key)};
  const TRes* result{asset.get()};
  _assets[key] = std::move(asset);
  return *result;
}

template <typename TResKey, typename TRes>
void asset_map<TResKey, TRes>::clear()
{
  _assets.clear();
}
}  // namespace eely