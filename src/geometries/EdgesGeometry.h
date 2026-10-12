#pragma once
#include "core/BufferGeometry.h"
class EdgesGeometry : public BufferGeometry {
  public:
    struct Parameters { std::shared_ptr<BufferGeometry> geometry; double thresholdAngle; };
    Parameters parameters;
    EdgesGeometry(std::shared_ptr<BufferGeometry> geometry = nullptr, double thresholdAngle = 1);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
};
