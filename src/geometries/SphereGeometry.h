#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class SphereGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radius;
        double widthSegments;
        double heightSegments;
        double phiStart;
        double phiLength;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    SphereGeometry(double radius = 1, double widthSegments = 32, double heightSegments = 16, double phiStart = 0, double phiLength = std::numbers::pi * 2, double thetaStart = 0, double thetaLength = std::numbers::pi);
    SphereGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<SphereGeometry> fromJSON(const JsonValue& data);
};
