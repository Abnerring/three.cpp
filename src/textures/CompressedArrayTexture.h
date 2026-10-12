#pragma once
#include "textures/CompressedTexture.h"
class CompressedArrayTexture : public CompressedTexture {
  public:
    double wrapR = ClampToEdgeWrapping;
    std::shared_ptr<std::vector<double>> layerUpdates = std::make_shared<std::vector<double>>();
    explicit CompressedArrayTexture(Mipmaps mipmaps = nullptr, double width = std::numeric_limits<double>::quiet_NaN(),
                                    double height = std::numeric_limits<double>::quiet_NaN(),
                                    double depth = std::numeric_limits<double>::quiet_NaN(),
                                    double format = RGBAFormat, double type = UnsignedByteType);
    Texture& copy(const Texture& source) override;
    std::shared_ptr<Texture> clone() const override;
    void addLayerUpdate(double layerIndex);
    void clearLayerUpdates();
};
