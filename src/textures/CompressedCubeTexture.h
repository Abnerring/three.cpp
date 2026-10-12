#pragma once
#include "textures/CompressedTexture.h"
class CompressedCubeTexture : public CompressedTexture {
  public:
    explicit CompressedCubeTexture(Image images = nullptr, double format = RGBAFormat, double type = UnsignedByteType);
    std::shared_ptr<Texture> clone() const override;
};
