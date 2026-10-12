#include "core/Raycaster.h"
#include "core/ArraySort.h"
#include "cameras/Camera.h"
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "core/Object3D.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void intersect(Object3D& object, const Raycaster& raycaster, std::vector<Intersection>& target,
               bool recursive) {
    auto keepAlive = object.weak_from_this().lock();
    bool propagate = true;
    if (object.layers.test(raycaster.layers))
        propagate = object.raycast(raycaster, target);
    if (propagate && recursive) {
        // Upstream retains the children array even if a callback replaces the field.
        const auto children = object.children;
        const auto length = children.size();
        for (std::size_t i = 0; i < length; ++i) {
            if (i >= children.size() || !children[i])
                throw JSTypeError("Cannot read layers of missing raycast child");
            auto child = children[i];
            intersect(*child, raycaster, target, true);
        }
    }
}
void sortIntersections(std::vector<Intersection>& target) {
    v8ArraySort(target,
        [](const Intersection& a, const Intersection& b) { return a.distance - b.distance; });
}
} // namespace

Raycaster::Raycaster(const Vector3& origin, const Vector3& direction, double near, double far)
    : ray(origin, direction), near(near), far(far) {}
void Raycaster::set(const Vector3& origin, const Vector3& direction) {
    ray.set(origin, direction);
}
void Raycaster::setFromCamera(const Vector2& coordinates, Camera& viewCamera) {
    if (dynamic_cast<PerspectiveCamera*>(&viewCamera)) {
        ray.origin.setFromMatrixPosition(viewCamera.matrixWorld);
        ray.direction.set(coordinates.x, coordinates.y, 0.5)
            .unproject(viewCamera)
            .sub(ray.origin)
            .normalize();
        camera = &viewCamera;
        cameraOwner_ = std::dynamic_pointer_cast<Camera>(viewCamera.weak_from_this().lock());
    } else if (dynamic_cast<OrthographicCamera*>(&viewCamera)) {
        ray.origin.set(coordinates.x, coordinates.y, viewCamera.projectionMatrix.elements()[14])
            .unproject(viewCamera);
        ray.direction.set(0, 0, -1).transformDirection(viewCamera.matrixWorld);
        camera = &viewCamera;
        cameraOwner_ = std::dynamic_pointer_cast<Camera>(viewCamera.weak_from_this().lock());
    } else
        std::cerr << "THREE.Raycaster: Unsupported camera type: " << viewCamera.type << '\n';
}
Raycaster& Raycaster::setFromXRController(const Object3D& controller) {
    Matrix4 matrix;
    matrix.identity().extractRotation(controller.matrixWorld);
    ray.origin.setFromMatrixPosition(controller.matrixWorld);
    ray.direction.set(0, 0, -1).applyMatrix4(matrix);
    return *this;
}
std::vector<Intersection>& Raycaster::intersectObject(Object3D& object, bool recursive,
                                                      std::vector<Intersection>& target) const {
    intersect(object, *this, target, recursive);
    sortIntersections(target);
    return target;
}
std::vector<Intersection> Raycaster::intersectObject(Object3D& object, bool recursive) const {
    std::vector<Intersection> target;
    intersectObject(object, recursive, target);
    return target;
}
std::vector<Intersection>&
Raycaster::intersectObjects(SharedArray<std::shared_ptr<Object3D>> objects, bool recursive,
                            std::vector<Intersection>& target) const {
    const auto length = objects.size();
    for (std::size_t i = 0; i < length; ++i) {
        if (i >= objects.size() || !objects[i])
            throw JSTypeError("Cannot read layers of missing raycast object");
        auto object = objects[i];
        intersect(*object, *this, target, recursive);
    }
    sortIntersections(target);
    return target;
}
std::vector<Intersection>
Raycaster::intersectObjects(SharedArray<std::shared_ptr<Object3D>> objects,
                            bool recursive) const {
    std::vector<Intersection> target;
    intersectObjects(objects, recursive, target);
    return target;
}
