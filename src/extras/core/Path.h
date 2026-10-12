#pragma once
#include "extras/core/CurvePath.h"
#include <numbers>

class Path : public CurvePath<Vector2> {
  public:
    using PointsPtr = std::shared_ptr<Points>;
    std::shared_ptr<Vector2> currentPoint = std::make_shared<Vector2>();
    explicit Path(const PointsPtr& points = nullptr);
    Path& setFromPoints(const Points& points);
    Path& setFromPoints(const PointsPtr& points) { return setFromPoints(*points); }
    Path& moveTo(double x, double y);
    Path& lineTo(double x, double y);
    Path& quadraticCurveTo(double cpx, double cpy, double x, double y);
    Path& bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
    Path& splineThru(const Points& points);
    Path& splineThru(const PointsPtr& points) { return splineThru(*points); }
    // Relative omitted x/y undergo JS undefined + currentPoint before reaching
    // EllipseCurve defaults, hence NaN rather than the documented zero defaults.
    Path& arc(double x = NAN, double y = NAN, double radius = 1, double start = 0,
              double end = 2 * std::numbers::pi, bool clockwise = false);
    Path& absarc(double x = 0, double y = 0, double radius = 1, double start = 0,
                 double end = 2 * std::numbers::pi, bool clockwise = false);
    Path& ellipse(double x = NAN, double y = NAN, double xRadius = 1, double yRadius = 1,
                  double start = 0, double end = 2 * std::numbers::pi, bool clockwise = false, double rotation = 0);
    Path& absellipse(double x = 0, double y = 0, double xRadius = 1, double yRadius = 1,
                     double start = 0, double end = 2 * std::numbers::pi, bool clockwise = false, double rotation = 0);
    std::shared_ptr<Curve<Vector2>> clone() const override;
    Path& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    Path& fromJSON(const JsonValue& json) override;
};
