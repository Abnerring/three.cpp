#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class CapsuleGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radius;
        double height;
        double capSegments;
        double radialSegments;
        double heightSegments;
    };
    Parameters parameters;
    CapsuleGeometry(double radius = 1, double height = 1, double capSegments = 4, double radialSegments = 8, double heightSegments = 1);
    CapsuleGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<CapsuleGeometry> fromJSON(const JsonValue& data);
};
