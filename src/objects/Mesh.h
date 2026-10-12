#pragma once
#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "materials/MeshBasicMaterial.h"
class Mesh : public Object3D {
  public:
    std::shared_ptr<BufferGeometry> geometry;
    std::shared_ptr<Material> material;
    SharedArray<std::shared_ptr<Material>> materials;
    bool materialIsArray = false;
    std::shared_ptr<std::map<std::string, std::size_t>> morphTargetDictionary;
    std::optional<SharedArray<double>> morphTargetInfluences;
    struct Count {
        JSValue value{1};
        operator double() const { return value.toNumber(); }
        Count& operator=(const JSValue& next) { value = next; return *this; }
        Count& operator=(const Count& next) { value = next.value; return *this; }
        const JSValue& asValue() const { return value; }
    };
    Count count;
    Mesh(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
         std::shared_ptr<Material> material = std::make_shared<MeshBasicMaterial>());
    Mesh(std::shared_ptr<BufferGeometry> geometry,
         std::vector<std::shared_ptr<Material>> materials);
    virtual void updateMorphTargets();
    virtual bool intersectsFrustum(const class Frustum& frustum);
    virtual Vector3& getVertexPosition(std::size_t index, Vector3& target) const;
    bool raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) override;
    virtual void _computeIntersections(const Raycaster& raycaster,
                                       std::vector<Intersection>& intersects,
                                       const class Ray& rayLocalSpace);
    Object3D& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Object3D::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;

  protected:
    Mesh(std::shared_ptr<BufferGeometry> geometry, std::shared_ptr<Material> material,
         bool initializeMorphTargets);
    Mesh(std::shared_ptr<BufferGeometry> geometry,
         std::vector<std::shared_ptr<Material>> materials, bool initializeMorphTargets);
    void serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const override;
    virtual void serializeBeforeGeometry(Object3DJSONContext&, JsonValue&) const {}
    virtual void serializeBeforeMaterial(Object3DJSONContext&, JsonValue&) const {}
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
