#pragma once
#include "geometries/CylinderGeometry.h"
#include <numbers>

class ConeGeometry : public CylinderGeometry {
  public:
    struct Parameters {
        double radius;
        double height;
        double radialSegments;
        double heightSegments;
        bool openEnded;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    ConeGeometry(double radius = 1, double height = 1, double radialSegments = 32, double heightSegments = 1, bool openEnded = false, double thetaStart = 0, double thetaLength = std::numbers::pi * 2);
    ConeGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<ConeGeometry> fromJSON(const JsonValue& data);
};
