#pragma once
#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "materials/MeshBasicMaterial.h"
class Mesh : public Object3D {
  public:
    std::shared_ptr<BufferGeometry> geometry;
    std::shared_ptr<Material> material;
    std::vector<std::shared_ptr<Material>> materials;
    bool materialIsArray = false;
    std::optional<std::map<std::string, std::size_t>> morphTargetDictionary;
    std::optional<std::vector<double>> morphTargetInfluences;
    std::size_t count = 1;
    Mesh(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
         std::shared_ptr<Material> material = std::make_shared<MeshBasicMaterial>());
    Mesh(std::shared_ptr<BufferGeometry> geometry,
         std::vector<std::shared_ptr<Material>> materials);
    void updateMorphTargets();
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
    void serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const override;
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
