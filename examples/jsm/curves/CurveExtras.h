#pragma once

// Native port of the pinned examples/jsm/curves/CurveExtras.js.
//
// Every class is a Curve<Vector3> whose getPoint() writes the analytic formula of the
// pinned source. Trigonometric values and powers come from JSNumberMath, which carries the
// pinned Node/V8 Math.sin, Math.cos and Math.pow semantics, so the samples stay comparable
// with the JavaScript module bit for bit on the pinned oracle.
#include "extras/core/Curve.h"

class GrannyKnot : public Curve<Vector3> {
  public:
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class HeartCurve : public Curve<Vector3> {
  public:
    double scale = 5;
    explicit HeartCurve(double scale = 5);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class VivianiCurve : public Curve<Vector3> {
  public:
    double scale = 70;
    explicit VivianiCurve(double scale = 70);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class KnotCurve : public Curve<Vector3> {
  public:
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class HelixCurve : public Curve<Vector3> {
  public:
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class TrefoilKnot : public Curve<Vector3> {
  public:
    double scale = 10;
    explicit TrefoilKnot(double scale = 10);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class TorusKnot : public Curve<Vector3> {
  public:
    double scale = 10;
    explicit TorusKnot(double scale = 10);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class CinquefoilKnot : public Curve<Vector3> {
  public:
    double scale = 10;
    explicit CinquefoilKnot(double scale = 10);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class TrefoilPolynomialKnot : public Curve<Vector3> {
  public:
    double scale = 10;
    explicit TrefoilPolynomialKnot(double scale = 10);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class FigureEightPolynomialKnot : public Curve<Vector3> {
  public:
    double scale = 1;
    explicit FigureEightPolynomialKnot(double scale = 1);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class DecoratedTorusKnot4a : public Curve<Vector3> {
  public:
    double scale = 40;
    explicit DecoratedTorusKnot4a(double scale = 40);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class DecoratedTorusKnot4b : public Curve<Vector3> {
  public:
    double scale = 40;
    explicit DecoratedTorusKnot4b(double scale = 40);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class DecoratedTorusKnot5a : public Curve<Vector3> {
  public:
    double scale = 40;
    explicit DecoratedTorusKnot5a(double scale = 40);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};

class DecoratedTorusKnot5c : public Curve<Vector3> {
  public:
    double scale = 40;
    explicit DecoratedTorusKnot5c(double scale = 40);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& optionalTarget) override;
};
