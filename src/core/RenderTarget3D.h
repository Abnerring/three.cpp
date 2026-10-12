#pragma once
#include "core/RenderTarget.h"
class RenderTarget3D : public RenderTarget {
  public:
    explicit RenderTarget3D(double width = 1, double height = 1, double depth = 1,
                            JSValue options = JSValue::object());
    std::shared_ptr<RenderTarget> clone() const override;
};
