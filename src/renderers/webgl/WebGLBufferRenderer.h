#pragma once
#include "core/JSValue.h"
#include <utility>

// Draw-command logic from WebGLBufferRenderer. GL, extensions and info are
// managed native adapter objects; the module does not create a browser context.
class WebGLBufferRenderer {
    JSValue object_;
    struct Existing {};
    WebGLBufferRenderer(JSValue object, Existing) : object_(std::move(object)) {}
    static void initialize(JSValue self, JSValue gl, JSValue extensions, JSValue info);
  public:
    explicit WebGLBufferRenderer(JSValue gl = {}, JSValue extensions = {}, JSValue info = {});
    static JSValue prototype();
    static WebGLBufferRenderer fromObject(JSValue object) { return {std::move(object), Existing{}}; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue setMode(JSValue value = {});
    JSValue render(JSValue start = {}, JSValue count = {});
    JSValue renderInstances(JSValue start = {}, JSValue count = {}, JSValue primcount = {});
    JSValue renderMultiDraw(JSValue starts = {}, JSValue counts = {}, JSValue drawCount = {});
};
