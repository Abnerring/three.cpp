#include "utils/SceneUtils.h"

#include "core/BufferGeometry.h"
#include "core/VertexAttribute.h"
#include "math/Matrix4.h"
#include "objects/Group.h"
#include "objects/InstancedMesh.h"
#include "objects/Mesh.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "core/InstancedBufferAttribute.h"

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
namespace {
struct AttributeSnapshot {
    std::shared_ptr<VertexAttribute> attribute;
    double itemSize = 0;
    std::vector<double> x, y, z, w;
};
}  // namespace

void SceneUtils::sortInstancedMesh(const std::shared_ptr<InstancedMesh>& mesh,
                                   const std::function<double(double, double)>& compareFn) {
    if (!mesh || !mesh->instanceMatrix || !mesh->geometry) return;
    const double count = mesh->count;

    // Snapshot the matrix payloads the way the pinned deepCloneAttribute does.
    std::vector<Matrix4> matrices(static_cast<std::size_t>(count));
    for (double i = 0; i < count; ++i) mesh->getMatrixAt(i, matrices[static_cast<std::size_t>(i)]);

    // Snapshot the optional instance colours.
    const bool hasColors = static_cast<bool>(mesh->instanceColor);
    std::vector<Color> colors;
    if (hasColors) {
        colors.resize(static_cast<std::size_t>(count));
        for (double i = 0; i < count; ++i) mesh->getColorAt(i, colors[static_cast<std::size_t>(i)]);
    }

    // Snapshot the instanced geometry attributes component by component.
    std::vector<AttributeSnapshot> snapshots;
    for (const auto& [name, attribute] : mesh->geometry->attributes) {
        (void) name;
        if (!attribute) continue;
        if (!std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute)) continue;
        AttributeSnapshot snapshot;
        snapshot.attribute = attribute;
        snapshot.itemSize = attribute->getItemSize();
        snapshot.x.resize(static_cast<std::size_t>(count));
        if (snapshot.itemSize > 1) snapshot.y.resize(static_cast<std::size_t>(count));
        if (snapshot.itemSize > 2) snapshot.z.resize(static_cast<std::size_t>(count));
        if (snapshot.itemSize > 3) snapshot.w.resize(static_cast<std::size_t>(count));
        for (double i = 0; i < count; ++i) {
            const auto index = static_cast<std::size_t>(i);
            snapshot.x[index] = attribute->getX(i);
            if (snapshot.itemSize > 1) snapshot.y[index] = attribute->getY(i);
            if (snapshot.itemSize > 2) snapshot.z[index] = attribute->getZ(i);
            if (snapshot.itemSize > 3) snapshot.w[index] = attribute->getW(i);
        }
        snapshots.push_back(std::move(snapshot));
    }

    std::vector<double> tokens(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < tokens.size(); ++i) tokens[i] = static_cast<double>(i);
    std::stable_sort(tokens.begin(), tokens.end(),
                    [&compareFn](double a, double b) { return compareFn(a, b) < 0; });

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto refIndex = static_cast<std::size_t>(tokens[i]);
        mesh->setMatrixAt(static_cast<double>(i), matrices[refIndex]);
        if (hasColors) mesh->setColorAt(static_cast<double>(i), colors[refIndex]);
        for (auto& snapshot : snapshots) {
            snapshot.attribute->setX(static_cast<double>(i), snapshot.x[refIndex]);
            if (snapshot.itemSize > 1) snapshot.attribute->setY(static_cast<double>(i), snapshot.y[refIndex]);
            if (snapshot.itemSize > 2) snapshot.attribute->setZ(static_cast<double>(i), snapshot.z[refIndex]);
            if (snapshot.itemSize > 3) snapshot.attribute->setW(static_cast<double>(i), snapshot.w[refIndex]);
        }
    }
}