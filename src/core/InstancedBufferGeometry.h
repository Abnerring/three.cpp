#pragma once
#include "core/BufferGeometry.h"

class InstancedBufferGeometry : public BufferGeometry {
  public:
    JSValue instanceCount = std::numeric_limits<double>::infinity();
    InstancedBufferGeometry();
    InstancedBufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
};
