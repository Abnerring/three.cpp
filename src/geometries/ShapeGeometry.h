#pragma once
#include "core/BufferGeometry.h"
#include "extras/core/Shape.h"

class ShapeGeometry : public BufferGeometry {
  public:
    using ShapeList = std::vector<std::shared_ptr<Shape>>;
    using Shapes = std::variant<std::shared_ptr<Shape>, std::shared_ptr<ShapeList>>;
    using ShapeMap = OrderedMap<std::shared_ptr<Shape>>;
    struct Parameters { Shapes shapes; double curveSegments; };
    Parameters parameters;
    static std::shared_ptr<Shape> defaultShape();
    ShapeGeometry(Shapes shapes = defaultShape(), double curveSegments = 12);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<ShapeGeometry> fromJSON(const JsonValue& data, const ShapeMap& shapes);
};
