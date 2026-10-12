#pragma once
#include "objects/Mesh.h"
#include "core/InstancedBufferAttribute.h"
#include "math/Box3.h"
#include "math/Sphere.h"
#include "textures/DataTexture.h"

class InstancedMesh : public Mesh {
  public:
    std::shared_ptr<InstancedBufferAttribute> instanceMatrix, instanceColor;
    std::shared_ptr<DataTexture> morphTexture;
    std::shared_ptr<Box3> boundingBox;
    std::shared_ptr<Sphere> boundingSphere;

    InstancedMesh(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
                  std::shared_ptr<Material> material = std::make_shared<MeshBasicMaterial>(),
                  JSValue count = JSValue());
    InstancedMesh(std::shared_ptr<BufferGeometry> geometry,
                  std::vector<std::shared_ptr<Material>> materials, JSValue count);
    virtual void computeBoundingBox();
    virtual void computeBoundingSphere();
    Object3D& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    Color& getColorAt(double index, Color& color) const;
    virtual Matrix4& getMatrixAt(double index, Matrix4& matrix) const;
    void getMorphAt(double index, Mesh& object) const;
    bool raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) override;
    InstancedMesh& setColorAt(double index, const Color& color);
    InstancedMesh& setMatrixAt(double index, const Matrix4& matrix);
    InstancedMesh& setMorphAt(double index, const Mesh& object);
    void updateMorphTargets() override {}
    void dispose() override;

  protected:
    void serializeBeforeGeometry(Object3DJSONContext& meta, JsonValue& object) const override;

  private:
    void initializeInstances(const JSValue& count);
};
