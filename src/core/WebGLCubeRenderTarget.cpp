#include "core/WebGLCubeRenderTarget.h"
#include "textures/CubeTexture.h"
WebGLCubeRenderTarget::WebGLCubeRenderTarget(double size, JSValue options)
    : RenderTarget(size, size, options) {
    TextureImage::Faces faces;
    faces.reserve(6);
    for (int face = 0; face < 6; ++face) {
        auto image = std::make_shared<TextureImage>();
        image->width = size; image->height = size; image->depth = 1;
        faces.push_back(std::move(image));
    }
    auto cube = std::make_shared<CubeTexture>(TextureImage::array(faces));
    cube->isRenderTargetTexture = true; cube->renderTarget = this;
    texture = cube;
    _setTextureOptions(options);
}
WebGLCubeRenderTarget& WebGLCubeRenderTarget::fromEquirectangularTexture(JSValue, JSValue) {
    throw std::logic_error(
        "WebGLCubeRenderTarget.fromEquirectangularTexture needs the renderer's equirectangular "
        "shader pass, which is not implemented yet");
}
WebGLCubeRenderTarget& WebGLCubeRenderTarget::clear(JSValue, bool, bool, bool) {
    throw std::logic_error(
        "WebGLCubeRenderTarget.clear needs per face render target binding, which is not "
        "implemented yet");
}
