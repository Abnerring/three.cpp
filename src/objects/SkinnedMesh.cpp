#include "objects/SkinnedMesh.h"
#include "core/Raycaster.h"
#include <iostream>

namespace {
// three.js shares these scratch values across calls, including callback reentry.
thread_local Vector3 skinVertex;
thread_local Sphere skinSphere;
thread_local Matrix4 skinInverse;
thread_local Ray skinRay;
std::shared_ptr<VertexAttribute> skinAttribute(const std::shared_ptr<BufferGeometry>& geometry,
                                     const char* name) {
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'attributes')");
    const auto attribute = geometry->getAttribute(name);
    if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
    return attribute;
}
void skinVectors(const std::shared_ptr<BufferGeometry>& geometry, double index, Vector4& indices, Vector4& weights) {
    indices.fromBufferAttribute(*skinAttribute(geometry, "skinIndex"), index);
    weights.fromBufferAttribute(*skinAttribute(geometry, "skinWeight"), index);
}
Matrix4 boneMatrix(const std::shared_ptr<Skeleton>& skeleton, double index) {
    if (!skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
    if (!std::isfinite(index) || index < 0 || std::floor(index) != index || index >= skeleton->bones.size())
        throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");
    const auto i = std::size_t(index);
    const auto bone = skeleton->bones.storage()->holes.contains(i) ? nullptr : skeleton->bones[i];
    if (!bone) throw JSTypeError("Cannot read properties of null (reading 'matrixWorld')");
    if (i >= skeleton->boneInverses.size() || skeleton->boneInverses.storage()->holes.contains(i) || !skeleton->boneInverses[i])
        throw JSTypeError("Cannot read properties of undefined (reading 'elements')");
    Matrix4 matrix;
    matrix.multiplyMatrices(bone->matrixWorld, *skeleton->boneInverses[i]);
    return matrix;
}
}
SkinnedMesh::SkinnedMesh(std::shared_ptr<BufferGeometry> geometry, std::shared_ptr<Material> material)
    : Mesh(std::move(geometry), std::move(material)) { isSkinnedMesh = true; type = "SkinnedMesh"; }
SkinnedMesh::SkinnedMesh(std::shared_ptr<BufferGeometry> geometry,
                         std::vector<std::shared_ptr<Material>> materials)
    : Mesh(std::move(geometry), std::move(materials)) { isSkinnedMesh = true; type = "SkinnedMesh"; }
void SkinnedMesh::computeBoundingBox() {
    const auto currentGeometry = geometry;
    if (!boundingBox) boundingBox = std::make_shared<Box3>();
    boundingBox->makeEmpty();
    const auto position = skinAttribute(currentGeometry, "position");
    for (std::size_t i = 0; double(i) < position->getCount(); ++i) {
        getVertexPosition(i, skinVertex);
        boundingBox->expandByPoint(skinVertex);
    }
}
void SkinnedMesh::computeBoundingSphere() {
    const auto currentGeometry = geometry;
    if (!boundingSphere) boundingSphere = std::make_shared<Sphere>();
    boundingSphere->makeEmpty();
    const auto position = skinAttribute(currentGeometry, "position");
    for (std::size_t i = 0; double(i) < position->getCount(); ++i) {
        getVertexPosition(i, skinVertex);
        boundingSphere->expandByPoint(skinVertex);
    }
}
Object3D& SkinnedMesh::copy(const Object3D& source, bool recursive) {
    const auto& other = dynamic_cast<const SkinnedMesh&>(source);
    Mesh::copy(source, recursive);
    bindMode = other.bindMode;
    bindMatrix.copy(other.bindMatrix);
    bindMatrixInverse.copy(other.bindMatrixInverse);
    skeleton = other.skeleton;
    boundingBox = other.boundingBox ? std::make_shared<Box3>(other.boundingBox->clone()) : nullptr;
    boundingSphere = other.boundingSphere ? std::make_shared<Sphere>(other.boundingSphere->clone()) : nullptr;
    return *this;
}
std::shared_ptr<Object3D> SkinnedMesh::clone(bool recursive) const {
    auto result = std::make_shared<SkinnedMesh>(); result->copy(*this, recursive); return result;
}
bool SkinnedMesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    if (!material && !(materialIsArray || !materials.empty())) return true;
    if (!boundingSphere) computeBoundingSphere();
    skinSphere.copy(*boundingSphere).applyMatrix4(matrixWorld);
    if (!raycaster.ray.intersectsSphere(skinSphere)) return true;
    skinInverse.copy(matrixWorld).invert();
    skinRay.copy(raycaster.ray).applyMatrix4(skinInverse);
    if (boundingBox && !skinRay.intersectsBox(*boundingBox)) return true;
    _computeIntersections(raycaster, intersects, skinRay);
    return true;
}
Vector3& SkinnedMesh::getVertexPosition(std::size_t index, Vector3& target) const {
    Mesh::getVertexPosition(index, target);
    applyBoneTransform(double(index), target);
    return target;
}
void SkinnedMesh::bind(std::shared_ptr<Skeleton> armature, const Matrix4* matrix) {
    skeleton = std::move(armature);
    if (!matrix) {
        updateMatrixWorld(true);
        if (!skeleton) throw JSTypeError("Cannot read properties of null (reading 'calculateInverses')");
        skeleton->calculateInverses();
        matrix = &matrixWorld;
    }
    bindMatrix.copy(*matrix);
    bindMatrixInverse.copy(*matrix).invert();
}
void SkinnedMesh::pose() {
    if (!skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'pose')");
    skeleton->pose();
}
void SkinnedMesh::normalizeSkinWeights() {
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'attributes')");
    const auto weight = geometry->getAttribute("skinWeight");
    if (!weight) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    Vector4 vector;
    const double length = weight->getCount();
    for (std::size_t i = 0; double(i) < length; ++i) {
        vector.fromBufferAttribute(*weight, double(i));
        const double scale = 1 / vector.manhattanLength();
        if (scale != std::numeric_limits<double>::infinity()) vector.multiplyScalar(scale);
        else vector.set(1, 0, 0, 0);
        weight->setXYZW(double(i), vector.x, vector.y, vector.z, vector.w);
    }
}
void SkinnedMesh::updateMatrixWorld(bool force) {
    Mesh::updateMatrixWorld(force);
    if (bindMode == AttachedBindMode) bindMatrixInverse.copy(matrixWorld).invert();
    else if (bindMode == DetachedBindMode) bindMatrixInverse.copy(bindMatrix).invert();
    else std::cerr << "THREE.SkinnedMesh: Unrecognized bindMode: " << bindMode << '\n';
}
Vector3& SkinnedMesh::applyBoneTransform(double index, Vector3& target) const {
    const auto armature = skeleton;
    const auto currentGeometry = geometry;
    Vector4 indices, weights; skinVectors(currentGeometry, index, indices, weights);
    Vector4 base(target.x, target.y, target.z, 1);
    target.set(0, 0, 0);
    base.applyMatrix4(bindMatrix);
    for (std::size_t i = 0; i < 4; ++i) {
        const double weight = weights.getComponent(i);
        if (weight == 0) continue;
        Vector4 vector; vector.copy(base).applyMatrix4(boneMatrix(armature, indices.getComponent(i)));
        target.x += vector.x * weight; target.y += vector.y * weight; target.z += vector.z * weight;
    }
    return target.applyMatrix4(bindMatrixInverse);
}
Vector4& SkinnedMesh::applyBoneTransform(double index, Vector4& target) const {
    const auto armature = skeleton;
    const auto currentGeometry = geometry;
    Vector4 indices, weights; skinVectors(currentGeometry, index, indices, weights);
    Vector4 base; base.copy(target);
    target.set(0, 0, 0, 0);
    base.applyMatrix4(bindMatrix);
    for (std::size_t i = 0; i < 4; ++i) {
        const double weight = weights.getComponent(i);
        if (weight == 0) continue;
        Vector4 vector; vector.copy(base).applyMatrix4(boneMatrix(armature, indices.getComponent(i)));
        target.addScaledVector(vector, weight);
    }
    target.w = base.w;
    return target.applyMatrix4(bindMatrixInverse);
}
void SkinnedMesh::serializeBeforeMaterial(Object3DJSONContext& meta, JsonValue& object) const {
    object["bindMode"] = bindMode;
    JsonValue::Array matrix;
    for (const auto value : bindMatrix.elements()) matrix.emplace_back(value);
    object["bindMatrix"] = std::move(matrix);
    if (skeleton) {
        const auto value = skeleton;
        meta.cacheResource("skeletons", value->uuid, [&] { return value->toJSON(); });
        object["skeleton"] = value->uuid;
    }
}
