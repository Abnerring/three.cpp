#pragma once
#include "core/BufferGeometry.h"
class BoxGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double width, height, depth, widthSegments, heightSegments, depthSegments;
    };
    Parameters parameters;
    BoxGeometry(double width = 1, double height = 1, double depth = 1, double widthSegments = 1,
                double heightSegments = 1, double depthSegments = 1);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
};
