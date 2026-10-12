#include "math/Frustum.h"
#include "math/Box3.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <stdexcept>
#include <string>

Frustum::Frustum(const Plane& p0, const Plane& p1, const Plane& p2, const Plane& p3,
                 const Plane& p4, const Plane& p5)
    : planes{p0, p1, p2, p3, p4, p5} {}
Frustum& Frustum::set(const Plane& p0, const Plane& p1, const Plane& p2, const Plane& p3,
                      const Plane& p4, const Plane& p5) {
    planes[0].copy(p0);
    planes[1].copy(p1);
    planes[2].copy(p2);
    planes[3].copy(p3);
    planes[4].copy(p4);
    planes[5].copy(p5);
    return *this;
}
Frustum& Frustum::copy(const Frustum& frustum) {
    for (size_t i = 0; i < 6; ++i)
        planes[i].copy(frustum.planes[i]);
    return *this;
}
Frustum& Frustum::setFromProjectionMatrix(const Matrix4& m, int coordinateSystem,
                                          bool reversedDepth) {
    const auto me = m.elements();
    const auto me0 = me[0], me1 = me[1], me2 = me[2], me3 = me[3];
    const auto me4 = me[4], me5 = me[5], me6 = me[6], me7 = me[7];
    const auto me8 = me[8], me9 = me[9], me10 = me[10], me11 = me[11];
    const auto me12 = me[12], me13 = me[13], me14 = me[14], me15 = me[15];
    planes[0].setComponents(me3 - me0, me7 - me4, me11 - me8, me15 - me12).normalize();
    planes[1].setComponents(me3 + me0, me7 + me4, me11 + me8, me15 + me12).normalize();
    planes[2].setComponents(me3 + me1, me7 + me5, me11 + me9, me15 + me13).normalize();
    planes[3].setComponents(me3 - me1, me7 - me5, me11 - me9, me15 - me13).normalize();
    if (reversedDepth) {
        planes[4].setComponents(me2, me6, me10, me14).normalize();
        planes[5].setComponents(me3 - me2, me7 - me6, me11 - me10, me15 - me14).normalize();
    } else {
        planes[4].setComponents(me3 - me2, me7 - me6, me11 - me10, me15 - me14).normalize();
        if (coordinateSystem == WebGLCoordinateSystem)
            planes[5].setComponents(me3 + me2, me7 + me6, me11 + me10, me15 + me14).normalize();
        else if (coordinateSystem == WebGPUCoordinateSystem)
            planes[5].setComponents(me2, me6, me10, me14).normalize();
        else
            throw std::invalid_argument(
                "THREE.Frustum.setFromProjectionMatrix(): Invalid coordinate system: " +
                std::to_string(coordinateSystem));
    }
    return *this;
}
Sphere& Frustum::sharedSphere() {
    static thread_local Sphere sphere;
    return sphere;
}
bool Frustum::intersectsObject(Object3D& object) const {
    auto* mesh = dynamic_cast<Mesh*>(&object);
    auto& sphere = sharedSphere();
    if (auto* skinned = dynamic_cast<SkinnedMesh*>(mesh)) {
        if (!skinned->boundingSphere) skinned->computeBoundingSphere();
        if (!skinned->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*skinned->boundingSphere);
    } else if (auto* instanced = dynamic_cast<InstancedMesh*>(mesh)) {
        if (!instanced->boundingSphere) instanced->computeBoundingSphere();
        if (!instanced->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*instanced->boundingSphere);
    } else {
        const auto geometry = mesh ? mesh->geometry
            : dynamic_cast<Line*>(&object) ? dynamic_cast<Line&>(object).geometry
            : dynamic_cast<Points*>(&object) ? dynamic_cast<Points&>(object).geometry
            : dynamic_cast<Sprite*>(&object) ? dynamic_cast<Sprite&>(object).geometry : nullptr;
        if (!geometry) throw JSTypeError("Cannot read properties of null or undefined (reading 'boundingSphere')");
        if (!geometry->boundingSphere) geometry->computeBoundingSphere();
        if (!geometry->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*geometry->boundingSphere);
    }
    sphere.applyMatrix4(object.matrixWorld);
    return intersectsSphere(sphere);
}
bool Frustum::intersectsSprite(const Vector2& center, const Matrix4& matrixWorld) const {
    auto& sphere = sharedSphere();
    sphere.center.set(0, 0, 0);
    const auto offset = Vector2(0.5, 0.5).distanceTo(center);
    sphere.radius = 0.7071067811865476 + offset;
    sphere.applyMatrix4(matrixWorld);
    return intersectsSphere(sphere);
}
bool Frustum::intersectsSphere(const Sphere& sphere) const {
    const auto& center = sphere.center;
    const auto negRadius = -sphere.radius;
    for (size_t i = 0; i < 6; ++i) {
        const auto distance = planes[i].distanceToPoint(center);
        if (distance < negRadius)
            return false;
    }
    return true;
}
bool Frustum::intersectsBox(const Box3& box) const {
    Vector3 vector;
    for (size_t i = 0; i < 6; ++i) {
        const auto& plane = planes[i];
        vector.setX(plane.normal.x > 0 ? box.max.x : box.min.x);
        vector.setY(plane.normal.y > 0 ? box.max.y : box.min.y);
        vector.setZ(plane.normal.z > 0 ? box.max.z : box.min.z);
        if (plane.distanceToPoint(vector) < 0)
            return false;
    }
    return true;
}
bool Frustum::containsPoint(const Vector3& point) const {
    for (size_t i = 0; i < 6; ++i)
        if (planes[i].distanceToPoint(point) < 0)
            return false;
    return true;
}
Frustum Frustum::clone() const {
    return Frustum().copy(*this);
}
