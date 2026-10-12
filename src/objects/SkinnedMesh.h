#pragma once
#include "objects/Mesh.h"
#include "objects/Skeleton.h"
#include "math/Box3.h"
#include "math/Sphere.h"
#include "math/Vector4.h"

class SkinnedMesh : public Mesh {
  public:
    std::string bindMode = AttachedBindMode;
    Matrix4 bindMatrix, bindMatrixInverse;
    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<Box3> boundingBox;
    std::shared_ptr<Sphere> boundingSphere;

    SkinnedMesh(std::shared_ptr<BufferGeometry> geometry = std::make_shared<BufferGeometry>(),
                std::shared_ptr<Material> material = std::make_shared<MeshBasicMaterial>());
    SkinnedMesh(std::shared_ptr<BufferGeometry> geometry,
                std::vector<std::shared_ptr<Material>> materials);
    virtual void computeBoundingBox();
    virtual void computeBoundingSphere();
    Object3D& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    bool raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) override;
    Vector3& getVertexPosition(std::size_t index, Vector3& target) const override;
    void bind(std::shared_ptr<Skeleton> skeleton, const Matrix4* bindMatrix = nullptr);
    void bind(std::shared_ptr<Skeleton> skeleton, const Matrix4& bindMatrix) {
        bind(std::move(skeleton), &bindMatrix);
    }
    void pose();
    void normalizeSkinWeights();
    void updateMatrixWorld(bool force = false) override;
    Vector3& applyBoneTransform(double index, Vector3& target) const;
    Vector4& applyBoneTransform(double index, Vector4& target) const;

  protected:
    void serializeBeforeMaterial(Object3DJSONContext& meta, JsonValue& object) const override;
};
