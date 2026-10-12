#pragma once
#include "core/BufferGeometry.h"

class PolyhedronGeometry : public BufferGeometry {
  public:
    using Numbers = std::vector<double>;
    using NumbersPtr = std::shared_ptr<Numbers>;
    struct Parameters { NumbersPtr vertices, indices; double radius, detail; };
    Parameters parameters;
    PolyhedronGeometry(NumbersPtr vertices = std::make_shared<Numbers>(),
                       NumbersPtr indices = std::make_shared<Numbers>(), double radius = 1, double detail = 0);
    PolyhedronGeometry(const Numbers& vertices, const Numbers& indices, double radius = 1, double detail = 0)
        : PolyhedronGeometry(std::make_shared<Numbers>(vertices), std::make_shared<Numbers>(indices), radius, detail) {}
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<PolyhedronGeometry> fromJSON(const JsonValue& data);
};
