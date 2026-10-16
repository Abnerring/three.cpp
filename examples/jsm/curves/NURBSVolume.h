#pragma once
#include "curves/NURBSUtils.h"
#include "math/Vector3.h"
#include <vector>

// Native port of the pinned examples/jsm/curves/NURBSVolume.js.
//
// Like the surface port, the constructor keeps only the
// `knots.length - degree - 1` sized block that the upstream loops copy and rebuilds each
// entry as a homogeneous Vector4; a missing control point is the upstream TypeError.
class NURBSVolume {
  public:
    static constexpr bool isNURBSVolume = true;
    double degree1 = 0;
    double degree2 = 0;
    double degree3 = 0;
    std::vector<double> knots1;
    std::vector<double> knots2;
    std::vector<double> knots3;
    NURBSUtils::ControlVolume controlPoints;

    NURBSVolume(double degree1 = 0, double degree2 = 0, double degree3 = 0, std::vector<double> knots1 = {},
                std::vector<double> knots2 = {}, std::vector<double> knots3 = {},
                const NURBSUtils::ControlVolume& controlPoints = {});
    // Writes the volume point for the interpolation factors t1, t2 and t3 into `target`.
    void getPoint(double t1, double t2, double t3, Vector3& target) const;
};
