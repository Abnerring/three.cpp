#include "utils/SceneUtils.h"

#include "core/BufferGeometry.h"
#include "core/VertexAttribute.h"
#include "math/Matrix4.h"
#include "objects/Group.h"
#include "objects/InstancedMesh.h"
#include "objects/Mesh.h"

#include <memory>
#include <vector>

std::shared_ptr<Group> SceneUtils::createMeshesFromInstancedMesh(
    const std::shared_ptr<InstancedMesh>& instancedMesh) {
    auto group = std::make_shared<Group>();
    if (!instancedMesh) return group;
    const double count = instancedMesh->count;
    for (double i = 0; i < count; ++i) {
        auto mesh = std::make_shared<Mesh>(instancedMesh->geometry, instancedMesh->material);
        instancedMesh->getMatrixAt(i, mesh->matrix);
        mesh->matrix.decompose(mesh->position, mesh->quaternion, mesh->scale);
        group->add(mesh);
    }
    group->copy(*instancedMesh);
    group->updateMatrixWorld();
    return group;
}

std::shared_ptr<Group> SceneUtils::createMultiMaterialObject(
    const std::shared_ptr<BufferGeometry>& geometry,
    const std::vector<std::shared_ptr<Material>>& materials) {
    auto group = std::make_shared<Group>();
    for (const auto& material : materials) {
        group->add(std::make_shared<Mesh>(geometry, material));
    }
    return group;
}

double SceneUtils::reduceVertices(const std::shared_ptr<Object3D>& object,
                                  const std::function<double(double, const Vector3&)>& callback,
                                  double initialValue) {
    double value = initialValue;
    if (!object) return value;
    Vector3 vertex;
    object->updateWorldMatrix(true, true);
    object->traverseVisible([&](Object3D& child) {
        const auto mesh = dynamic_cast<Mesh*>(&child);
        const auto geometry = mesh ? mesh->geometry : nullptr;
        if (!geometry) return;
        const auto position = geometry->getAttribute("position");
        if (!position) return;
        for (double i = 0; i < position->getCount(); ++i) {
            if (child.isMesh && mesh) {
                mesh->getVertexPosition(static_cast<std::size_t>(i), vertex);
            } else {
                vertex.fromBufferAttribute(*position, i);
            }
            if (!child.isSkinnedMesh) vertex.applyMatrix4(child.matrixWorld);
            value = callback(value, vertex);
        }
    });
    return value;
}
