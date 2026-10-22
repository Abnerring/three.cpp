#pragma once
#include "core/RenderTarget.h"

// Upstream WebGLCubeRenderTarget (r187): a render target whose texture is a six
// face CubeTexture. Constructing it and its texture options is portable; the two
// methods that need GPU work say so instead of pretending to work.
class WebGLCubeRenderTarget : public RenderTarget {
  public:
    explicit WebGLCubeRenderTarget(double size = 1, JSValue options = JSValue::object());
    const bool isWebGLCubeRenderTarget = true;
    // Upstream renders an equirectangular texture into the six faces with a shader.
    WebGLCubeRenderTarget& fromEquirectangularTexture(JSValue renderer, JSValue texture);
    // Upstream clears face by face through renderer.setRenderTarget(this, face).
    WebGLCubeRenderTarget& clear(JSValue renderer, bool color = true, bool depth = true,
                                 bool stencil = true);
};
