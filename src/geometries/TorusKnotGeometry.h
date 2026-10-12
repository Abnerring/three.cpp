#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class TorusKnotGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radius;
        double tube;
        double tubularSegments;
        double radialSegments;
        double p;
        double q;
    };
    Parameters parameters;
    TorusKnotGeometry(double radius = 1, double tube = 0.4, double tubularSegments = 64, double radialSegments = 8, double p = 2, double q = 3);
    TorusKnotGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<TorusKnotGeometry> fromJSON(const JsonValue& data);
};
