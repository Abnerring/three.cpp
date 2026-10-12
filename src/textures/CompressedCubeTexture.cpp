#include "textures/CompressedCubeTexture.h"
namespace {
double faceDimension(const Texture::Image& images, bool height) {
    if (!images || images->faces.empty() || !images->faces[0])
        throw JSTypeError("Cannot read properties of undefined (reading 'width')");
    return height ? images->faces[0]->height : images->faces[0]->width;
}
}
CompressedCubeTexture::CompressedCubeTexture(Image images, double format, double type)
    : CompressedTexture(nullptr, faceDimension(images, false), faceDimension(images, true), format, type, CubeReflectionMapping) {
    isCompressedCubeTexture = true; isCubeTexture = true; image = std::move(images);
}
std::shared_ptr<Texture> CompressedCubeTexture::clone() const {
    // Upstream new this.constructor() has no images argument and throws before copy.
    auto result = std::make_shared<CompressedCubeTexture>(); result->copy(*this); return result;
}
