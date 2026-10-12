#pragma once
#include "core/JSValue.h"
#include <utility>

// Indexed draw-command logic using injected managed native adapter objects.
// setIndex captures the type and bytesPerElement values, as the upstream closure does.
class WebGLIndexedBufferRenderer {
    JSValue object_;
    struct Existing {};
    WebGLIndexedBufferRenderer(JSValue object, Existing) : object_(std::move(object)) {}
    static void initialize(JSValue self, JSValue gl, JSValue extensions, JSValue info);
  public:
    explicit WebGLIndexedBufferRenderer(JSValue gl = {}, JSValue extensions = {}, JSValue info = {});
    static JSValue prototype();
    static WebGLIndexedBufferRenderer fromObject(JSValue object) { return {std::move(object), Existing{}}; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue setMode(JSValue value = {});
    JSValue setIndex(JSValue value = {});
    JSValue render(JSValue start = {}, JSValue count = {});
    JSValue renderInstances(JSValue start = {}, JSValue count = {}, JSValue primcount = {});
    JSValue renderMultiDraw(JSValue starts = {}, JSValue counts = {}, JSValue drawCount = {});
};
