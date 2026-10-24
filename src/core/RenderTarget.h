#pragma once
#include "core/EventDispatcher.h"
#include "core/SharedArray.h"
#include "math/Vector4.h"
#include "textures/Texture.h"

// Canonical managed constructor. The numerical facade below remains available
// to the existing renderer; its GPU resource bridge is separate.
JSValue nativeRenderTargetConstructor();

class RenderTarget : public EventDispatcher {
  public:
    struct TextureProperty {
        RenderTarget* owner;
        std::shared_ptr<Texture> get() const;
        operator std::shared_ptr<Texture>() const { return get(); }
        Texture* operator->() const;
        TextureProperty& operator=(std::shared_ptr<Texture> value);
        TextureProperty& operator=(const TextureProperty& value) { return *this = value.get(); }
    };
    struct DepthTextureProperty {
        RenderTarget* owner;
        std::shared_ptr<Texture> get() const { return owner->_depthTexture; }
        operator std::shared_ptr<Texture>() const { return get(); }
        Texture* operator->() const { return get().get(); }
        explicit operator bool() const { return bool(get()); }
        DepthTextureProperty& operator=(std::shared_ptr<Texture> value);
        DepthTextureProperty& operator=(const DepthTextureProperty& value) { return *this = value.get(); }
    };
    const bool isRenderTarget = true;
    bool isRenderTarget3D = false;
    double width, height, depth;
    Vector4 scissor, viewport;
    bool scissorTest = false;
    SharedArray<std::shared_ptr<Texture>> textures;
    TextureProperty texture{this};
    bool depthBuffer = true, stencilBuffer = false;
    bool resolveColorBuffer = true, resolveDepthBuffer = true, resolveStencilBuffer = true;
    bool storeMultisampledColorBuffer = true, storeMultisampledDepthBuffer = true,
         storeMultisampledStencilBuffer = true;
    std::shared_ptr<Texture> _depthTexture;
    DepthTextureProperty depthTexture{this};
    double samples = 0;
    bool multiview = false, useArrayDepthTexture = false;

    explicit RenderTarget(double width = 1, double height = 1, JSValue options = JSValue::object());
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    virtual void _setTextureOptions(JSValue options = JSValue::object());
    virtual void setSize(double width, double height, double depth = 1);
    virtual std::shared_ptr<RenderTarget> clone() const;
    virtual RenderTarget& copy(const RenderTarget& source);
    virtual void dispose();
};
