#pragma once

#include "math/Vector3.h"

#include <vector>

// Native port of examples/jsm/utils/GeometryUtils.js from the pinned three.js revision
// (see upstream.json).
//
// Three pure point generators used by the demos: the recursive 2D and 3D Hilbert curves and the
// Gosper (flowsnake) curve. They only depend on Vector3, so the port is a straight transcription of
// the pinned recursion with the same argument order, including the duplicated corner indices that
// drive each Hilbert sub-curve.
namespace GeometryUtils {

std::vector<Vector3> hilbert2D(const Vector3& center = Vector3(0, 0, 0), double size = 10,
                               int iterations = 1, int v0 = 0, int v1 = 1, int v2 = 2,
                               int v3 = 3);

std::vector<Vector3> hilbert3D(const Vector3& center = Vector3(0, 0, 0), double size = 10,
                               int iterations = 1, int v0 = 0, int v1 = 1, int v2 = 2,
                               int v3 = 3, int v4 = 4, int v5 = 5, int v6 = 6, int v7 = 7);

// Flat xyz triples, starting at the origin like the pinned path array.
std::vector<double> gosper(double size = 1);

}  // namespace GeometryUtils
