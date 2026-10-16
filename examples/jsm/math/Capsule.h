#pragma once
#include "math/Vector3.h"

class Box3;

// Native port of examples/jsm/math/Capsule.js from the pinned three.js revision
// (see upstream.json).
//
// A capsule is a cylinder with hemispherical caps at both ends, i.e. a swept sphere. Like
// every other math class of the library, `start` and `end` are value members instead of
// shared Vector3 references (docs/PORTING.md), and the injected `start` / `end` arguments of
// the constructor and `set()` are copied. `clone()` reproduces
// `new this.constructor().copy( this )`, so the default values of the constructor are
// visible before the copy overwrites them and the result never aliases the source.
//
// `intersectsBox()` is the only intersection test of this revision; it decomposes the test
// into the three axis pairs of the helper `checkAABBAxis`, which stays module scope.

class Capsule {
  public:
    /** The start vector. */
    Vector3 start;

    /** The end vector. */
    Vector3 end;

    /** The capsule's radius. */
    double radius;

    Capsule(const Vector3& start = Vector3(0, 0, 0), const Vector3& end = Vector3(0, 1, 0),
            double radius = 1);

    /** Returns a new capsule with copied values from this instance. */
    Capsule clone() const;

    /** Sets the capsule components to the given values (the vectors are copied). */
    Capsule& set(const Vector3& start, const Vector3& end, double radius);

    /** Copies the values of the given capsule to this instance. */
    Capsule& copy(const Capsule& capsule);

    /** Returns the center point of this capsule. */
    Vector3& getCenter(Vector3& target) const;

    /** Adds the given offset to this capsule, effectively moving it in 3D space. */
    Capsule& translate(const Vector3& v);

    /** Returns `true` if the given bounding box intersects with this capsule. */
    bool intersectsBox(const Box3& box) const;
};
