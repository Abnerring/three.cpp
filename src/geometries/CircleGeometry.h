#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class CircleGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radius;
        double segments;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    CircleGeometry(double radius = 1, double segments = 32, double thetaStart = 0, double thetaLength = std::numbers::pi * 2);
    CircleGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<CircleGeometry> fromJSON(const JsonValue& data);
};
