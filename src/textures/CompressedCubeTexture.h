#pragma once
#include "textures/CompressedTexture.h"
class CompressedCubeTexture : public CompressedTexture {
  public:
    explicit CompressedCubeTexture(Image images = nullptr, double format = RGBAFormat, double type = UnsignedByteType);
    std::shared_ptr<Texture> clone() const override;
};

#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeCompressedCubeTextureConstructor();
JSValue nativeCompressedCubeTextureModule();
JSValue nativeCompressedCubeTextureBinding(const std::string& name);
