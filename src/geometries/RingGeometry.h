#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class RingGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double innerRadius;
        double outerRadius;
        double thetaSegments;
        double phiSegments;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    RingGeometry(double innerRadius = 0.5, double outerRadius = 1, double thetaSegments = 32, double phiSegments = 1, double thetaStart = 0, double thetaLength = std::numbers::pi * 2);
    RingGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<RingGeometry> fromJSON(const JsonValue& data);
};
