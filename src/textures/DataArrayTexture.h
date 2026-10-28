#pragma once
#include "textures/Texture.h"
JSValue nativeDataArrayTextureConstructor();
JSValue nativeDataArrayTextureModule();
JSValue nativeDataArrayTextureBinding(const std::string& name);
class DataArrayTexture : public Texture {
  public:
    double wrapR = ClampToEdgeWrapping;
    // JS Set insertion order, SameValueZero equality, and identity survive clear().
    std::shared_ptr<std::vector<double>> layerUpdates = std::make_shared<std::vector<double>>();
    explicit DataArrayTexture(TextureImage::Data data = nullptr, double width = 1, double height = 1, double depth = 1);
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
    void addLayerUpdate(double layerIndex);
    void clearLayerUpdates();
};
