#pragma once
#include "core/JSValue.h"
#include <utility>

// Pinned three.js CPU material-to-uniform protocol. GPU material shaders and
// texture/sampler binding are supplied separately by the renderer.
JSValue nativeWebGLMaterialsConstructor();
class WebGLMaterials {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return value;
    }
    static JSValue take(JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return std::move(value);
    }
    WebGLMaterials(const JSValue& value, Existing) : object_(copy(value)) {}

  public:
    WebGLMaterials(const JSValue& renderer = {}, const JSValue& properties = {});
    WebGLMaterials(const WebGLMaterials& other) : object_(copy(other.object_)) {}
    WebGLMaterials(WebGLMaterials&& other) : object_(take(other.object_)) {}
    WebGLMaterials& operator=(const WebGLMaterials& other) {
        static_cast<void>(JSWeakValue(object_));
        object_ = copy(other.object_);
        return *this;
    }
    WebGLMaterials& operator=(WebGLMaterials&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other)
            object_ = take(other.object_);
        return *this;
    }
    static WebGLMaterials fromObject(const JSValue& value) { return {value, Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue refreshFogUniforms(const JSValue& uniforms = {}, const JSValue& fog = {});
    JSValue refreshMaterialUniforms(const JSValue& uniforms = {}, const JSValue& material = {},
                                    const JSValue& pixelRatio = {}, const JSValue& height = {},
                                    const JSValue& transmissionRenderTarget = {});
};
