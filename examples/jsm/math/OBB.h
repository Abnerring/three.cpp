#pragma once
#include "math/Matrix3.h"
#include "math/Vector3.h"
#include <limits>

class Box3;
class Sphere;
class Ray;
class Plane;
class Matrix4;

// Native port of examples/jsm/math/OBB.js from the pinned three.js revision (see
// upstream.json).
//
// The source keeps the two axis records `a` / `b`, the rotation tables `R` / `AbsR`, the
// translation vector `t` and a set of loose temporaries (`xAxis`, `yAxis`, `zAxis`, `v1`,
// `size`, `closestPoint`, `rotationMatrix`, `aabb`, `matrix`, `inverse`, `localRay`, `obb`)
// in module scope and reuses them for every call. The native port keeps exactly one copy of
// that module state per owning thread in the translation unit.
//
// `center`, `halfSize` and `rotation` are value members, so `set()` copies the arguments
// instead of aliasing them (docs/PORTING.md). `intersectRay()` follows the nullable-target
// convention of the existing `Ray::intersectBox()`: it returns the caller's target on a hit
// and `nullptr` on a miss, so `intersectsRay()` stays expressible as `!= nullptr`.

class OBB {
  public:
    /** The center of the OBB. */
    Vector3 center;

    /** Positive halfwidth extents of the OBB along each axis. */
    Vector3 halfSize;

    /** The rotation of the OBB. */
    Matrix3 rotation;

    OBB(const Vector3& center = Vector3(), const Vector3& halfSize = Vector3(),
        const Matrix3& rotation = Matrix3());

    /** Sets the OBBs components to the given values (the objects are copied). */
    OBB& set(const Vector3& center, const Vector3& halfSize, const Matrix3& rotation);

    /** Copies the values of the given OBB to this instance. */
    OBB& copy(const OBB& obb);

    /** Returns a new OBB with copied values from this instance. */
    OBB clone() const;

    /** Returns the size of this OBB. */
    Vector3& getSize(Vector3& target) const;

    /** Clamps the given point within the bounds of this OBB. */
    Vector3& clampPoint(const Vector3& point, Vector3& target) const;

    /** Returns `true` if the given point lies within this OBB. */
    bool containsPoint(const Vector3& point) const;

    /** Returns `true` if the given AABB intersects this OBB. */
    bool intersectsBox3(const Box3& box3) const;

    /** Returns `true` if the given bounding sphere intersects this OBB. */
    bool intersectsSphere(const Sphere& sphere) const;

    /** Returns `true` if the given OBB intersects this OBB. */
    bool intersectsOBB(const OBB& obb, double epsilon = std::numeric_limits<double>::epsilon()) const;

    /** Returns `true` if the given plane intersects this OBB. */
    bool intersectsPlane(const Plane& plane) const;

    /** Performs a ray/OBB intersection test and stores the intersection point in `target`. */
    Vector3* intersectRay(const Ray& ray, Vector3& target) const;

    /** Returns `true` if the given ray intersects this OBB. */
    bool intersectsRay(const Ray& ray) const;

    /** Defines an OBB based on the given AABB. */
    OBB& fromBox3(const Box3& box3);

    /** Returns `true` if the given OBB is equal to this OBB. */
    bool equals(const OBB& obb) const;

    /** Applies the given transformation matrix to this OBB. */
    OBB& applyMatrix4(const Matrix4& matrix);
};
