#pragma once

#include "common/OrderedMap.h"
#include "core/BufferGeometry.h"
#include "core/EventDispatcher.h"
#include "core/Layers.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Material;
class AnimationClip;
class VulkanRenderer;
class Scene;
class Camera;
class Raycaster;
struct Intersection;

// One context per document, explicitly shared with descendants like three.js' meta argument.
// Custom object serializers can register resources without serializing an existing UUID twice.
class Object3DJSONContext {
  public:
    OrderedMap<JsonValue> geometries, materials, textures, images, shapes, skeletons, animations,
        nodes;
    void cacheResource(const std::string& library, std::string uuid,
                       const std::function<JsonValue()>& serialize);
    void extractCaches(JsonValue& output);

  private:
    OrderedMap<JsonValue>& library(const std::string& name);
};

class Object3D : public EventDispatcher, public std::enable_shared_from_this<Object3D> {
  public:
    static Vector3 DEFAULT_UP;
    static bool DEFAULT_MATRIX_AUTO_UPDATE;
    static bool DEFAULT_MATRIX_WORLD_AUTO_UPDATE;

    const bool isObject3D = true;
    bool isCamera = false, isLight = false, isGroup = false, isScene = false, isMesh = false;
    const std::uint64_t id;
    std::string uuid, name, type = "Object3D";
    // Parent never owns a child through this pointer. Children own descendants.
    Object3D* parent = nullptr;
    std::vector<std::shared_ptr<Object3D>> children;
    Vector3 up, position, scale{1, 1, 1};
    Euler rotation;
    Quaternion quaternion;
    Matrix4 modelViewMatrix, matrix, matrixWorld;
    Matrix3 normalMatrix;
    bool matrixAutoUpdate, matrixWorldAutoUpdate, matrixWorldNeedsUpdate = false;
    Layers layers;
    bool visible = true, castShadow = false, receiveShadow = false, frustumCulled = true;
    int renderOrder = 0;
    bool isStatic = false; // JavaScript property "static" is a C++ keyword.
    std::optional<Vector3> pivot;
    std::vector<std::shared_ptr<AnimationClip>> animations;
    std::shared_ptr<Material> customDepthMaterial, customDistanceMaterial;
    JsonValue userData = JsonValue::Object{};
    using RenderCallback = std::function<void(VulkanRenderer&, Scene&, Camera&, BufferGeometry*,
                                              Material*, BufferGeometry::Group*)>;
    using ShadowCallback =
        std::function<void(VulkanRenderer&, Object3D&, Camera&, Camera&, BufferGeometry&, Material&,
                           BufferGeometry::Group*)>;
    RenderCallback onBeforeRender, onAfterRender;
    ShadowCallback onBeforeShadow, onAfterShadow;

    Object3D();
    ~Object3D() override;
    Object3D(const Object3D&) = delete;
    Object3D& operator=(const Object3D&) = delete;
    Object3D(Object3D&&) = delete;
    Object3D& operator=(Object3D&&) = delete;

    void applyMatrix4(const Matrix4& transform);
    Object3D& applyQuaternion(const Quaternion& q);
    void setRotationFromAxisAngle(const Vector3& axis, HIGH_PRECISION angle);
    void setRotationFromEuler(const Euler& euler);
    void setRotationFromMatrix(const Matrix4& m);
    void setRotationFromQuaternion(const Quaternion& q);
    Object3D& rotateOnAxis(const Vector3& axis, HIGH_PRECISION angle);
    Object3D& rotateOnWorldAxis(const Vector3& axis, HIGH_PRECISION angle);
    Object3D& rotateX(HIGH_PRECISION angle);
    Object3D& rotateY(HIGH_PRECISION angle);
    Object3D& rotateZ(HIGH_PRECISION angle);
    Object3D& translateOnAxis(const Vector3& axis, HIGH_PRECISION distance);
    Object3D& translateX(HIGH_PRECISION distance);
    Object3D& translateY(HIGH_PRECISION distance);
    Object3D& translateZ(HIGH_PRECISION distance);
    Vector3& localToWorld(Vector3& vector);
    Vector3& worldToLocal(Vector3& vector);
    void lookAt(const Vector3& target);
    void lookAt(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);
    Object3D& add(std::shared_ptr<Object3D> object);
    template <class... Rest>
        requires(sizeof...(Rest) > 0)
    Object3D& add(std::shared_ptr<Object3D> object, Rest... rest) {
        add(std::move(object));
        (add(std::move(rest)), ...);
        return *this;
    }
    Object3D& remove(Object3D* object);
    Object3D& remove(const std::shared_ptr<Object3D>& object) { return remove(object.get()); }
    Object3D& removeFromParent();
    Object3D& clear();
    Object3D& attach(std::shared_ptr<Object3D> object);
    Object3D* getObjectById(std::uint64_t objectId);
    Object3D* getObjectByName(const std::string& objectName);
    Object3D* getObjectByProperty(const std::string& property, const JsonValue& value);
    std::vector<Object3D*>& getObjectsByProperty(const std::string& property,
                                                 const JsonValue& value,
                                                 std::vector<Object3D*>& result);
    std::vector<Object3D*> getObjectsByProperty(const std::string& property,
                                                const JsonValue& value) {
        std::vector<Object3D*> result;
        getObjectsByProperty(property, value, result);
        return result;
    }
    Vector3& getWorldPosition(Vector3& target);
    Quaternion& getWorldQuaternion(Quaternion& target);
    Vector3& getWorldScale(Vector3& target);
    virtual Vector3& getWorldDirection(Vector3& target);
    void traverse(const std::function<void(Object3D&)>& callback);
    void traverseVisible(const std::function<void(Object3D&)>& callback);
    void traverseAncestors(const std::function<void(Object3D&)>& callback);
    void updateMatrix();
    virtual void updateMatrixWorld(bool force = false);
    virtual void updateWorldMatrix(bool updateParents = false, bool updateChildren = false,
                                   bool force = false);
    virtual std::shared_ptr<Object3D> clone(bool recursive = true) const;
    virtual Object3D& copy(const Object3D& source, bool recursive = true);
    // true also represents the upstream undefined result; false stops descendant queries.
    virtual bool raycast(const Raycaster&, std::vector<Intersection>&) { return true; }
    virtual JsonValue toJSON(Object3DJSONContext* meta = nullptr) const;
    JsonValue toJSON(bool root) const;
    void dispose();

  protected:
    virtual void serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const;
    virtual std::optional<JsonValue> propertyValue(const std::string& property) const;

  private:
    void checkChild(const std::shared_ptr<Object3D>& object) const;
};
