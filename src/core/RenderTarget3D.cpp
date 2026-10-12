#include "core/RenderTarget3D.h"
#include "textures/Data3DTexture.h"
RenderTarget3D::RenderTarget3D(double width, double height, double depth, JSValue options)
    : RenderTarget(width, height, options) {
    isRenderTarget3D = true;
    this->depth = depth;
    for (std::size_t i = 0; i < textures.size(); ++i) {
        auto current = std::make_shared<Data3DTexture>(nullptr, width, height, depth);
        current->isRenderTargetTexture = true; current->renderTarget = this;
        textures[i] = std::move(current);
    }
    _setTextureOptions(options);
}
std::shared_ptr<RenderTarget> RenderTarget3D::clone() const {
    auto result = std::make_shared<RenderTarget3D>(); result->copy(*this); return result;
}
