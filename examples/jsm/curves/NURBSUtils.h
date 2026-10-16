#pragma once

// Native port of the pinned examples/jsm/curves/NURBSUtils.js.
//
// The module is the reference implementation of The NURBS Book algorithms A2.2, A2.3,
// A3.1, A3.2, A4.2 and A4.3 used by the NURBS curve, surface and volume addons. Knot
// vectors, control points and parametric values keep the upstream binary64 arithmetic and
// evaluation order; a knot read out of range behaves like the JavaScript `undefined`
// entry (NaN), while an out-of-range control point is the upstream TypeError.
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <vector>

namespace NURBSUtils {

using KnotVector = std::vector<double>;
using ControlPoints = std::vector<Vector4>;
using ControlGrid = std::vector<ControlPoints>;
using ControlVolume = std::vector<ControlGrid>;

// Finds the knot vector span.
double findSpan(double p, double u, const KnotVector& U);
// Calculates basis functions: The NURBS Book, page 70, algorithm A2.2.
std::vector<double> calcBasisFunctions(double span, double u, double p, const KnotVector& U);
// Calculates B-Spline curve points: The NURBS Book, page 82, algorithm A3.1.
Vector4 calcBSplinePoint(double p, const KnotVector& U, const ControlPoints& P, double u);
// Calculates basis function derivatives: The NURBS Book, page 72, algorithm A2.3.
std::vector<std::vector<double>> calcBasisFunctionDerivatives(double span, double u, double p, double n,
                                                             const KnotVector& U);
// Calculates derivatives of a B-Spline: The NURBS Book, page 93, algorithm A3.2.
std::vector<Vector4> calcBSplineDerivatives(double p, const KnotVector& U, const ControlPoints& P, double u, double nd);
// Calculates "K over I": k! / ( i! ( k - i )! ).
double calcKoverI(double k, double i);
// Calculates the derivatives (0..nd) of a rational curve: The NURBS Book, page 127, algorithm A4.2.
std::vector<Vector3> calcRationalCurveDerivatives(const std::vector<Vector4>& Pders);
// Calculates NURBS curve derivatives: The NURBS Book, page 127, algorithm A4.2.
std::vector<Vector3> calcNURBSDerivatives(double p, const KnotVector& U, const ControlPoints& P, double u, double nd);
// Calculates a rational B-Spline surface point: The NURBS Book, page 134, algorithm A4.3.
void calcSurfacePoint(double p, double q, const KnotVector& U, const KnotVector& V, const ControlGrid& P,
                      double u, double v, Vector3& target);
// Calculates a rational B-Spline volume point: The NURBS Book, page 134, algorithm A4.3.
void calcVolumePoint(double p, double q, double r, const KnotVector& U, const KnotVector& V, const KnotVector& W,
                     const ControlVolume& P, double u, double v, double w, Vector3& target);

} // namespace NURBSUtils
