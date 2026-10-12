#include "textures/DataArrayTexture.h"
#include <cmath>
DataArrayTexture::DataArrayTexture(TextureImage::Data data, double width, double height, double depth) : Texture(nullptr) {
    isDataArrayTexture = true;
    image = std::make_shared<TextureImage>(std::move(data), width, height, depth);
    magFilter = NearestFilter; minFilter = NearestFilter;
    generateMipmaps = false; flipY = false; unpackAlignment = 1;
}
Texture& DataArrayTexture::copy(const Texture& source) {
    Texture::copy(source); wrapR = dynamic_cast<const DataArrayTexture&>(source).wrapR; return *this;
}
std::shared_ptr<Texture> DataArrayTexture::clone() const {
    auto result = std::make_shared<DataArrayTexture>(); result->copy(*this); return result;
}
void DataArrayTexture::addLayerUpdate(double layerIndex) {
    for (double value : *layerUpdates)
        if (value == layerIndex || (std::isnan(value) && std::isnan(layerIndex))) return;
    layerUpdates->push_back(layerIndex == 0 ? 0 : layerIndex);
}
void DataArrayTexture::clearLayerUpdates() { layerUpdates->clear(); }
