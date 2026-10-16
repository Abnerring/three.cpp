#pragma once
#include "curves/NURBSUtils.h"
#include "math/Vector3.h"
#include <vector>

// Native port of the pinned examples/jsm/curves/NURBSSurface.js.
//
// The constructor keeps only the `knots.length - degree - 1` by
// `knots.length - degree - 1` block of control points that the upstream loops copy, and
// rebuilds each entry as a homogeneous Vector4. A control point the loops read but the
// caller did not provide is the upstream TypeError.
class NURBSSurface {
  public:
    static constexpr bool isNURBSSurface = true;
    double degree1 = 0;
    double degree2 = 0;
    std::vector<double> knots1;
    std::vector<double> knots2;
    NURBSUtils::ControlGrid controlPoints;

    NURBSSurface(double degree1 = 0, double degree2 = 0, std::vector<double> knots1 = {},
                 std::vector<double> knots2 = {}, const NURBSUtils::ControlGrid& controlPoints = {});
    // Writes the surface point for the interpolation factors t1 and t2 into `target`.
    void getPoint(double t1, double t2, Vector3& target) const;
};
