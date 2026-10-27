#pragma once
#include "textures/Texture.h"
JSValue nativeFramebufferTextureConstructor();
JSValue nativeFramebufferTextureModule();
JSValue nativeFramebufferTextureBinding(const std::string& name);
class FramebufferTexture : public Texture {
  public:
    explicit FramebufferTexture(double width = std::numeric_limits<double>::quiet_NaN(),
                                double height = std::numeric_limits<double>::quiet_NaN());
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
};
