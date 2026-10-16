#pragma once
#include "core/BufferGeometry.h"
#include <vector>

// Native port of the pinned examples/jsm/geometries/ConvexGeometry.js.
//
// The upstream constructor only builds the position and normal attributes and never assigns
// `type` or `parameters`, so this port also leaves those BufferGeometry fields at their
// defaults and does not override copy()/clone(), like the pinned class.
class ConvexGeometry : public BufferGeometry {
  public:
    /** `points = []`: builds the convex hull of the given points in 3D space. */
    explicit ConvexGeometry(const std::vector<Vector3>& points = std::vector<Vector3>());
};
