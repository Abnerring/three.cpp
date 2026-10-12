#pragma once
#include "core/Object3D.h"
#include "materials/LineBasicMaterial.h"

class Line : public Object3D {
  public:
    std::shared_ptr<BufferGeometry> geometry;
    std::shared_ptr<Material> material;
    SharedArray<std::shared_ptr<Material>> materials;
    bool materialIsArray = false;
    std::shared_ptr<std::map<std::string,std::size_t>> morphTargetDictionary;
    std::optional<SharedArray<double>> morphTargetInfluences;
    Line(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
         std::shared_ptr<Material> material = std::make_shared<LineBasicMaterial>());
    Line(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials);
    Line& copy(const Object3D& source,bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    virtual Line& computeLineDistances();
    virtual void updateMorphTargets();
    virtual bool intersectsFrustum(const class Frustum& frustum);
    bool raycast(const Raycaster& raycaster,std::vector<Intersection>& intersects) override;
  protected:
    void serializeObjectResources(Object3DJSONContext& meta,JsonValue& object) const override;
};
