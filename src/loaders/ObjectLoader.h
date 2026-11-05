#pragma once
#include "core/JSValue.h"
#include "core/JSMember.h"
#include "core/OrderedMap.h"
#include "extras/core/Shape.h"
#include "geometries/ShapeGeometry.h"
#include "textures/TextureSource.h"
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

JSValue nativeObjectLoaderModule();
JSValue nativeObjectLoaderBinding(const std::string& name);
JSValue nativeObjectLoaderConstructor();

// The managed object exposes the source ObjectLoader pipeline, including images,
// textures and intrinsic async methods. The typed Result/parseGeometry facade is
// retained for callers using the earlier typed object graph.
class ObjectLoader {
  public:
    explicit ObjectLoader(JSValue manager = {});
    static JSValue prototype();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSMember manager{&object_, "manager"}, crossOrigin{&object_, "crossOrigin"};
    JSMember withCredentials{&object_, "withCredentials"}, path{&object_, "path"};
    JSMember resourcePath{&object_, "resourcePath"}, requestHeader{&object_, "requestHeader"};
    JSValue load(JSValue url = {}, JSValue onLoad = {}, JSValue onProgress = {}, JSValue onError = {});
    JSValue loadAsync(JSValue url, JSValue onProgress = {});
    JSValue parseManaged(JSValue json, JSValue onLoad = {});
    JSValue parseAsync(JSValue json);
    ObjectLoader& setCrossOrigin(JSValue value);
    ObjectLoader& setWithCredentials(JSValue value);
    ObjectLoader& setPath(JSValue value);
    ObjectLoader& setResourcePath(JSValue value);
    ObjectLoader& setRequestHeader(JSValue value);
    static void registerGeometry(JSValue type, JSValue geometryClass);
    struct Result {
        std::shared_ptr<Object3D> object;
        std::vector<std::shared_ptr<AnimationClip>> animations;
    };
    Result parse(const JSValue& json);

  private:
    JSValue object_;
    std::shared_ptr<Object3D> parseObject(const JSValue& data);
    std::map<std::string, std::shared_ptr<BufferGeometry>> geometries_;
    std::map<std::string, std::shared_ptr<Material>> materials_;
    // Kept in an ordered map so the material loader receives them in document order.
    // Native image descriptors, keyed by the uuid the document gave them. A document whose image is a URL
    // rather than a data record needs a host decoder and is refused by name.
    std::map<std::string, TextureImage::Ptr> images_;
    // The shapes a document declares, keyed by uuid, which a shape geometry refers to by that uuid.
    ShapeGeometry::ShapeMap shapes_;
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
