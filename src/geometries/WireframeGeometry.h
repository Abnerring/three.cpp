#pragma once
#include "core/BufferGeometry.h"
class WireframeGeometry : public BufferGeometry {
  public:
    struct Parameters { std::shared_ptr<BufferGeometry> geometry; };
    Parameters parameters;
    WireframeGeometry(std::shared_ptr<BufferGeometry> geometry = nullptr);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
};
