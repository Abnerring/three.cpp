#pragma once
#include "core/BufferGeometry.h"
#include <numbers>

class CylinderGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double radiusTop;
        double radiusBottom;
        double height;
        double radialSegments;
        double heightSegments;
        bool openEnded;
        double thetaStart;
        double thetaLength;
    };
    Parameters parameters;
    CylinderGeometry(double radiusTop = 1, double radiusBottom = 1, double height = 1, double radialSegments = 32, double heightSegments = 1, bool openEnded = false, double thetaStart = 0, double thetaLength = std::numbers::pi * 2);
    CylinderGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<CylinderGeometry> fromJSON(const JsonValue& data);
};
