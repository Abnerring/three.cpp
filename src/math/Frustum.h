#pragma once
#include "constants.h"
#include "core/JSValue.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include "math/Vector2.h"
#include <array>
#include <type_traits>
class Matrix4;
class Box3;
class Object3D;
class Frustum {
  public:
    std::array<Plane, 6> planes;
    Frustum(const Plane& p0 = Plane(), const Plane& p1 = Plane(), const Plane& p2 = Plane(),
            const Plane& p3 = Plane(), const Plane& p4 = Plane(), const Plane& p5 = Plane());
    Frustum& set(const Plane& p0, const Plane& p1, const Plane& p2, const Plane& p3,
                 const Plane& p4, const Plane& p5);
    Frustum& copy(const Frustum& frustum);
    Frustum& setFromProjectionMatrix(const Matrix4& m, int coordinateSystem = WebGLCoordinateSystem,
                                     bool reversedDepth = false);
    bool intersectsObject(Object3D& object) const;
    // Structural overload preserves both upstream bounds ownership branches.
    template <class Object>
        requires(
            requires(Object& o) {
                o.boundingSphere;
                o.computeBoundingSphere();
                o.matrixWorld;
            } ||
            requires(Object& o) {
                o.geometry;
                o.matrixWorld;
            })
    bool intersectsObject(Object& object) const {
        if constexpr (std::is_base_of_v<Object3D,Object>) return intersectsObject(static_cast<Object3D&>(object));
        auto& sphere = sharedSphere();
        if constexpr (requires { object.boundingSphere; }) {
            if (!object.boundingSphere)
                object.computeBoundingSphere();
            if (!object.boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
            sphere.copy(*object.boundingSphere).applyMatrix4(object.matrixWorld);
        } else {
            if (!object.geometry) throw JSTypeError("Cannot read properties of null (reading 'boundingSphere')");
            const auto geometry = object.geometry;
            if (!geometry->boundingSphere)
                geometry->computeBoundingSphere();
            if (!geometry->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
            sphere.copy(*geometry->boundingSphere).applyMatrix4(object.matrixWorld);
        }
        return intersectsSphere(sphere);
    }
    bool intersectsSprite(const Vector2& center, const Matrix4& matrixWorld) const;
    template <class Sprite> bool intersectsSprite(const Sprite& sprite) const {
        return intersectsSprite(sprite.center, sprite.matrixWorld);
    }
    bool intersectsSphere(const Sphere& sphere) const;
    bool intersectsBox(const Box3& box) const;
    bool containsPoint(const Vector3& point) const;
    Frustum clone() const;
  private:
    static Sphere& sharedSphere();
};
