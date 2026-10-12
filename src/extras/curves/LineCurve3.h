#pragma once
#include "extras/core/Curve.h"

class LineCurve3 : public Curve<Vector3> {
  public:
    static constexpr bool isLineCurve3 = true;
    std::shared_ptr<Vector3> v1, v2;

    explicit LineCurve3(std::shared_ptr<Vector3> v1 = std::make_shared<Vector3>(),
                           std::shared_ptr<Vector3> v2 = std::make_shared<Vector3>());
    using Curve<Vector3>::getPoint;
    Vector3& getPoint(double t, Vector3& target) override;
    using Curve<Vector3>::getPointAt;
    using Curve<Vector3>::getTangent;
    using Curve<Vector3>::getTangentAt;
    Vector3& getPointAt(double u, Vector3& target) override;
    Vector3& getTangent(double t, Vector3& target) override;
    Vector3& getTangentAt(double u, Vector3& target) override;
    std::shared_ptr<Curve<Vector3>> clone() const override;
    LineCurve3& copy(const Curve<Vector3>& source) override;
    JsonValue toJSON() const override;
    LineCurve3& fromJSON(const JsonValue& json) override;
};
