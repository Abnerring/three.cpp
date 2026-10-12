#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class TorusGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radius;
        double tube;
        double radialSegments;
        double tubularSegments;
        double arc;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    TorusGeometry(double radius = 1, double tube = 0.4, double radialSegments = 12, double tubularSegments = 48, double arc = std::numbers::pi * 2, double thetaStart = 0, double thetaLength = std::numbers::pi * 2);
    TorusGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<TorusGeometry> fromJSON(const JsonValue& data);
};
