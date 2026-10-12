#pragma once
#include "extras/core/Curve.h"

class CatmullRomCurve3 : public Curve<Vector3> {
  public:
    static constexpr bool isCatmullRomCurve3 = true;
    using Points = std::vector<std::shared_ptr<Vector3>>;
    using PointsPtr = std::shared_ptr<Points>;
    PointsPtr points;
    bool closed;
    std::string curveType;
    double tension;

    explicit CatmullRomCurve3(PointsPtr points = std::make_shared<Points>(), bool closed = false,
                             std::string curveType = "centripetal", double tension = 0.5);
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& target) override;
    std::shared_ptr<Curve<Vector3>> clone() const override;
    CatmullRomCurve3& copy(const Curve<Vector3>& source) override;
    JsonValue toJSON() const override;
    CatmullRomCurve3& fromJSON(const JsonValue& json) override;
};
