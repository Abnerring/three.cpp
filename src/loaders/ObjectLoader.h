#pragma once
#include "core/JSValue.h"
#include "core/OrderedMap.h"
#include <map>
#include <utility>
#include <memory>
#include <string>
#include <vector>

class Object3D;
class AnimationClip;
class BufferGeometry;
class Material;
class Texture;
class Skeleton;

// Port of src/loaders/ObjectLoader.js, first slice.
//
// The pipeline order is upstream's: animations, geometries, materials, then the object tree, because the
// later stages resolve by the uuid the earlier ones recorded. Images and textures are not part of this
// slice, so parseTextures is absent and a document that needs a texture is refused rather than silently
// losing it. The epilogue that every object shares is ported line for line from the upstream tail.
class ObjectLoader {
  public:
    struct Result {
        std::shared_ptr<Object3D> object;
        std::vector<std::shared_ptr<AnimationClip>> animations;
    };
    Result parse(const JSValue& json);

  private:
    std::shared_ptr<Object3D> parseObject(const JSValue& data);
    std::map<std::string, std::shared_ptr<BufferGeometry>> geometries_;
    std::map<std::string, std::shared_ptr<Material>> materials_;
    // Kept in an ordered map so the material loader receives them in document order.
    OrderedMap<std::shared_ptr<Texture>> textures_;
    // A skinned mesh records the skeleton uuid the document gave it and is bound once every skeleton exists,
    // which is the same two step shape the light targets use.
    std::map<std::string, std::shared_ptr<Skeleton>> skeletons_;
    std::vector<std::pair<std::shared_ptr<Object3D>, std::string>> pendingSkeletons_;
    void bindSkeletons(const std::shared_ptr<Object3D>& root);
    std::map<std::string, std::shared_ptr<AnimationClip>> clips_;
    // Upstream stores the target of a directional or spot light as the uuid string and binds it after the
    // whole tree exists, because the target may be a sibling that has not been parsed yet. Same two steps here.
    std::vector<std::pair<std::shared_ptr<Object3D>, std::string>> pendingTargets_;
    void bindLightTargets(const std::shared_ptr<Object3D>& root);
};