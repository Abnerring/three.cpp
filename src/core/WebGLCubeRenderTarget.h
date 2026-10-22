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
    // Upstream remembers the current target, clears each of the six faces through
    // renderer.setRenderTarget(this, face) plus renderer.clear(), then restores the
    // previous target. The renderer type is a template parameter so this header does
    // not need to know about any particular backend.
    template <class Renderer>
    WebGLCubeRenderTarget& clear(Renderer& renderer, const std::shared_ptr<WebGLCubeRenderTarget>& self,
                                 bool color = true, bool depth = true, bool stencil = true) {
        const auto current = renderer.getRenderTarget();
        for (std::uint32_t face = 0; face < 6; ++face) {
            renderer.setRenderTarget(self, face);
            // The backend has no stencil attachment yet, so stencil is accepted for
            // signature fidelity but not passed on.
            renderer.clear(color, depth);
        }
        renderer.setRenderTarget(current);
        return *this;
    }
    // A JS renderer handle cannot be unwrapped into a backend, so this overload reports
    // that instead of doing nothing.
    WebGLCubeRenderTarget& clear(JSValue renderer, bool color = true, bool depth = true,
                                 bool stencil = true);
};
