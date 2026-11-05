#pragma once
#include "textures/Texture.h"
JSValue nativeData3DTextureConstructor();
JSValue nativeData3DTextureModule();
JSValue nativeData3DTextureBinding(const std::string& name);
class Data3DTexture : public Texture {
  public:
    double wrapR = ClampToEdgeWrapping;
    explicit Data3DTexture(TextureImage::Data data = nullptr, double width = 1, double height = 1, double depth = 1);
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
};
