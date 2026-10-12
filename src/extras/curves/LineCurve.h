#pragma once
#include "extras/core/Curve.h"

class LineCurve : public Curve<Vector2> {
  public:
    static constexpr bool isLineCurve = true;
    std::shared_ptr<Vector2> v1, v2;

    explicit LineCurve(std::shared_ptr<Vector2> v1 = std::make_shared<Vector2>(),
                           std::shared_ptr<Vector2> v2 = std::make_shared<Vector2>());
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override;
    using Curve<Vector2>::getPointAt;
    using Curve<Vector2>::getTangent;
    using Curve<Vector2>::getTangentAt;
    Vector2& getPointAt(double u, Vector2& target) override;
    Vector2& getTangent(double t, Vector2& target) override;
    Vector2& getTangentAt(double u, Vector2& target) override;
    std::shared_ptr<Curve<Vector2>> clone() const override;
    LineCurve& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    LineCurve& fromJSON(const JsonValue& json) override;
};
