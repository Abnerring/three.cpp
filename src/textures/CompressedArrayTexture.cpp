#include "textures/CompressedArrayTexture.h"
#include <cmath>
CompressedArrayTexture::CompressedArrayTexture(Mipmaps mipmaps, double width, double height, double depth, double format, double type)
    : CompressedTexture(std::move(mipmaps), width, height, format, type) {
    isCompressedArrayTexture = true; image->depth = depth;
}
Texture& CompressedArrayTexture::copy(const Texture& source) {
    CompressedTexture::copy(source); wrapR = dynamic_cast<const CompressedArrayTexture&>(source).wrapR; return *this;
}
std::shared_ptr<Texture> CompressedArrayTexture::clone() const {
    auto result = std::make_shared<CompressedArrayTexture>(); result->copy(*this); return result;
}
void CompressedArrayTexture::addLayerUpdate(double layerIndex) {
    for (double value : *layerUpdates)
        if (value == layerIndex || (std::isnan(value) && std::isnan(layerIndex))) return;
    layerUpdates->push_back(layerIndex == 0 ? 0 : layerIndex);
}
void CompressedArrayTexture::clearLayerUpdates() { layerUpdates->clear(); }
