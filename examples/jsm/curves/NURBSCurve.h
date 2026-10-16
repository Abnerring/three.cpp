#pragma once
#include "curves/NURBSUtils.h"
#include "extras/core/Curve.h"
#include <vector>

// Native port of the pinned examples/jsm/curves/NURBSCurve.js.
//
// Control points are stored as homogeneous Vector4 values, exactly like the upstream
// constructor which rebuilds every entry with `new Vector4( point.x, point.y, point.z,
// point.w )`. `startKnot` and `endKnot` keep the upstream `||` fallback, so the optional
// arguments are managed values: an omitted, zero, NaN or otherwise falsy index selects the
// default.
class NURBSCurve : public Curve<Vector3> {
  public:
    static constexpr bool isNURBSCurve = true;
    double degree = 0;
    std::vector<double> knots;
    std::vector<Vector4> controlPoints;
    double startKnot = 0;
    double endKnot = 0;

    NURBSCurve(double degree = 0, std::vector<double> knots = {}, std::vector<Vector4> controlPoints = {},
               JSValue startKnot = {}, JSValue endKnot = {});
    using Curve<Vector3>::getPoint;
    using Curve<Vector3>::getTangent;
    Vector3& getPoint(double t, Vector3& target) override;
    Vector3& getTangent(double t, Vector3& target) override;
    std::shared_ptr<Curve<Vector3>> clone() const override;
    NURBSCurve& copy(const Curve<Vector3>& source) override;
    JsonValue toJSON() const override;
    NURBSCurve& fromJSON(const JsonValue& json) override;
};
