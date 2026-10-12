#pragma once
#include "extras/core/Path.h"

class Shape : public Path {
  public:
    using Holes = std::vector<std::shared_ptr<Path>>;
    std::string uuid;
    std::shared_ptr<Holes> holes = std::make_shared<Holes>();
    struct ExtractedPoints { Points shape; std::vector<Points> holes; };
    explicit Shape(const PointsPtr& points = nullptr);
    virtual std::vector<Points> getPointsHoles();
    virtual std::vector<Points> getPointsHoles(double divisions);
    virtual ExtractedPoints extractPoints();
    virtual ExtractedPoints extractPoints(double divisions);
    std::shared_ptr<Curve<Vector2>> clone() const override;
    Shape& copy(const Curve<Vector2>& source) override;
    JsonValue toJSON() const override;
    Shape& fromJSON(const JsonValue& json) override;
};
