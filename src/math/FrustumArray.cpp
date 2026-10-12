#include "math/FrustumArray.h"
#include "cameras/ArrayCamera.h"
#include "objects/Sprite.h"

namespace {
thread_local Matrix4 projScreenMatrix;
std::shared_ptr<Frustum> frustumAt(const std::vector<std::shared_ptr<Frustum>>& frustums, std::size_t index) {
    if (index >= frustums.size() || !frustums[index])
        throw JSTypeError("Cannot read properties of null or undefined (reading frustum method)");
    return frustums[index];
}
}
FrustumArray& FrustumArray::setFromArrayCamera(const ArrayCamera& cameraArray) {
    const auto cameras = cameraArray.cameras;
    for (std::size_t i = 0; i < cameras.size(); ++i) {
        const auto camera = cameras[i];
        if (!camera) throw JSTypeError("Cannot read properties of null or undefined (reading 'projectionMatrix')");
        projScreenMatrix.multiplyMatrices(camera->projectionMatrix, camera->matrixWorldInverse);
        if (_frustums.size() <= i) _frustums.resize(i + 1);
        if (!_frustums[i]) _frustums[i] = std::make_shared<Frustum>();
        _frustums[i]->setFromProjectionMatrix(projScreenMatrix, camera->coordinateSystem, camera->reversedDepth);
    }
    _count = cameras.size();
    return *this;
}
bool FrustumArray::intersectsObject(Object3D& object) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsObject(object)) return true;
    return false;
}
bool FrustumArray::intersectsSprite(const Sprite& sprite) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsSprite(sprite)) return true;
    return false;
}
bool FrustumArray::intersectsSphere(const Sphere& sphere) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsSphere(sphere)) return true;
    return false;
}
bool FrustumArray::intersectsBox(const Box3& box) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsBox(box)) return true;
    return false;
}
bool FrustumArray::containsPoint(const Vector3& point) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->containsPoint(point)) return true;
    return false;
}
FrustumArray& FrustumArray::copy(const FrustumArray& source) {
    coordinateSystem = source.coordinateSystem;
    for (std::size_t i = 0; i < source._count; ++i) {
        if (_frustums.size() <= i) _frustums.resize(i + 1);
        if (!_frustums[i]) _frustums[i] = std::make_shared<Frustum>();
        _frustums[i]->copy(*frustumAt(source._frustums,i));
    }
    _count = source._count;
    return *this;
}
FrustumArray FrustumArray::clone() const { return FrustumArray().copy(*this); }
