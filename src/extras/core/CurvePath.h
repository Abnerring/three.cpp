#pragma once
#include "extras/core/Curve.h"

// Curves and output points retain JavaScript reference identity. Vector is a
// static C++ adaptation: a path contains either Vector2 or Vector3 curves.
template<class Vector = Vector3> class CurvePath : public Curve<Vector> {
  public:
    using Curves = std::vector<std::shared_ptr<Curve<Vector>>>;
    using CurvesPtr = std::shared_ptr<Curves>;
    using Points = std::vector<std::shared_ptr<Vector>>;
    CurvesPtr curves = std::make_shared<Curves>();
    bool autoClose = false;
    std::shared_ptr<std::vector<double>> cacheLengths;

    CurvePath();
    void add(const std::shared_ptr<Curve<Vector>>& curve);
    CurvePath& closePath();
    using Curve<Vector>::getPoint;
    Vector& getPoint(double t, Vector& target) override;
    // The reference-returning Curve API cannot express upstream's null result.
    // These nullable overloads preserve that result without modifying target.
    Vector* getPointOrNull(double t, Vector& target);
    std::shared_ptr<Vector> getPointOrNull(double t);
    double getLength() override;
    void updateArcLengths() override;
    std::shared_ptr<std::vector<double>> getCurveLengths();
    Points getSpacedPoints() override { return getSpacedPoints(40); }
    Points getSpacedPoints(double divisions) override;
    Points getPoints() override { return getPoints(12); }
    Points getPoints(double divisions) override;
    std::shared_ptr<Curve<Vector>> clone() const override;
    CurvePath& copy(const Curve<Vector>& source) override;
    JsonValue toJSON() const override;
    CurvePath& fromJSON(const JsonValue& json) override;
};

extern template class CurvePath<Vector2>;
extern template class CurvePath<Vector3>;
