#pragma once
#include "core/JSValue.h"

// Pinned three.js state/cache protocol. Commands are supplied by a native
// renderer adapter; this class creates no OpenGL context or browser bridge.
JSValue nativeWebGLStateConstructor();
class WebGLState {
    JSValue object_;
    static JSValue checked(const JSValue& value) { static_cast<void>(JSWeakValue(value)); return value; }
    JSValue invoke(const char* name, const std::vector<JSValue>& arguments = {}) const;
  public:
    WebGLState(const JSValue& gl = {}, const JSValue& extensions = {});
    JSValue object() const { return checked(object_); }
    operator JSValue() const { return object(); }
    JSValue buffers() const { return object().get("buffers"); }
    JSValue enable(const JSValue& id = {});
    JSValue disable(const JSValue& id = {});
    JSValue bindFramebuffer(const JSValue& target = {}, const JSValue& framebuffer = {});
    JSValue drawBuffers(const JSValue& target = {}, const JSValue& framebuffer = {});
    JSValue useProgram(const JSValue& program = {});
    JSValue setBlending(const JSValue& blending = {}, const JSValue& equation = {}, const JSValue& src = {}, const JSValue& dst = {},
                       const JSValue& equationAlpha = {}, const JSValue& srcAlpha = {}, const JSValue& dstAlpha = {},
                       const JSValue& color = {}, const JSValue& alpha = {}, const JSValue& premultiplied = {});
    JSValue setMaterial(const JSValue& material = {}, const JSValue& frontFaceCW = {});
    JSValue setFlipSided(const JSValue& value = {});
    JSValue setCullFace(const JSValue& value = {});
    JSValue setLineWidth(const JSValue& value = {});
    JSValue setPolygonOffset(const JSValue& enabled = {}, const JSValue& factor = {}, const JSValue& units = {});
    JSValue setScissorTest(const JSValue& value = {});
    JSValue activeTexture(const JSValue& slot = {});
    JSValue bindTexture(const JSValue& type = {}, const JSValue& texture = {}, const JSValue& slot = {});
    JSValue unbindTexture();
    JSValue pixelStorei(const JSValue& name = {}, const JSValue& value = {});
    JSValue getParameter(const JSValue& name = {});
    JSValue updateUBOMapping(const JSValue& group = {}, const JSValue& program = {});
    JSValue uniformBlockBinding(const JSValue& group = {}, const JSValue& program = {}, const JSValue& buffer = {});
    JSValue scissor(const JSValue& value = {});
    JSValue viewport(const JSValue& value = {});
    JSValue reset();
#define THREE_WEBGL_STATE_TEXTURE_COMMAND(name) JSValue name(const std::vector<JSValue>& arguments = {});
    THREE_WEBGL_STATE_TEXTURE_COMMAND(compressedTexImage2D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(compressedTexImage3D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texImage2D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texImage3D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texStorage2D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texStorage3D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texSubImage2D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(texSubImage3D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(compressedTexSubImage2D)
    THREE_WEBGL_STATE_TEXTURE_COMMAND(compressedTexSubImage3D)
#undef THREE_WEBGL_STATE_TEXTURE_COMMAND
};
