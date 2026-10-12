#pragma once
#include "textures/Texture.h"
class ExternalTexture : public Texture {
  public:
    std::shared_ptr<void> sourceTexture;
    explicit ExternalTexture(std::shared_ptr<void> sourceTexture = nullptr);
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
};
