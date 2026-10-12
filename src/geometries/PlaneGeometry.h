#pragma once
#include "core/BufferGeometry.h"
class PlaneGeometry : public BufferGeometry {
  public:
    struct Parameters {
        double width, height, widthSegments, heightSegments;
    };
    Parameters parameters;
    PlaneGeometry(double width = 1, double height = 1, double widthSegments = 1,
                  double heightSegments = 1);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<PlaneGeometry> fromJSON(const JsonValue& json);
};
