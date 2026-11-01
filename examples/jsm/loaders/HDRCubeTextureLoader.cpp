#include "loaders/HDRCubeTextureLoader.h"

std::shared_ptr<CubeTexture> HDRCubeTextureLoader::createTexture() const {
    auto texture = std::make_shared<CubeTexture>();
    texture->type = _type;

    // Both float cases do the same four assignments, and a type that is neither falls through
    // without touching anything.
    if (_type == HDRLoader::kFloatType || _type == HDRLoader::kHalfFloatType) {
        texture->colorSpace = LinearSRGBColorSpace;
        texture->minFilter = LinearFilter;
        texture->magFilter = LinearFilter;
        texture->generateMipmaps = false;
    }
    return texture;
}

bool HDRCubeTextureLoader::setFace(CubeTexture& texture, int index,
                                   const HDRLoader::Result& result) const {
    if (index < 0 || index > 5) return false;
    // The face keeps the cube texture's settings, which is what upstream copies across.
    auto image = std::make_shared<TextureImage>(
        Uint8Array(std::vector<std::uint8_t>(result.data.size(), 0)), result.header.width,
        result.header.height);
    if (texture.images.get() == nullptr) texture.images = TextureImage::array();
    auto& faces = texture.images->faces;
    if (faces.size() < 6) faces.resize(6);
    faces[static_cast<std::size_t>(index)] = image;
    // Six faces in place means the texture can be uploaded.
    bool complete = true;
    for (const auto& face : faces)
        if (!face) complete = false;
    if (complete) texture.needsUpdate = true;
    return true;
}
