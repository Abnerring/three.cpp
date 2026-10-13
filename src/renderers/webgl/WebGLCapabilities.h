#pragma once
#include "core/JSMember.h"
#include <utility>

// Pinned three.js CPU protocol over an injected native GPU command object.
// This module does not determine or advertise Vulkan device capabilities.
JSValue nativeWebGLCapabilitiesConstructor();
class WebGLCapabilities {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value) { static_cast<void>(JSWeakValue(value)); return value; }
    static JSValue take(JSValue& value) { static_cast<void>(JSWeakValue(value)); return std::move(value); }
    WebGLCapabilities(const JSValue& value, Existing) : object_(copy(value)) {}
  public:
    explicit WebGLCapabilities(const JSValue& gl = {}, const JSValue& extensions = {}, const JSValue& parameters = {}, const JSValue& utils = {});
    WebGLCapabilities(const WebGLCapabilities& other) : object_(copy(other.object_)) {}
    WebGLCapabilities(WebGLCapabilities&& other) : object_(take(other.object_)) {}
    WebGLCapabilities& operator=(const WebGLCapabilities& other) { static_cast<void>(JSWeakValue(object_)); object_=copy(other.object_); return *this; }
    WebGLCapabilities& operator=(WebGLCapabilities&& other) { static_cast<void>(JSWeakValue(object_)); if(this!=&other)object_=take(other.object_); return *this; }
    static WebGLCapabilities fromObject(const JSValue& value) { return {value,Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSMember isWebGL2{&object_,"isWebGL2"};
    JSMember precision{&object_,"precision"};
    JSMember logarithmicDepthBuffer{&object_,"logarithmicDepthBuffer"};
    JSMember reversedDepthBuffer{&object_,"reversedDepthBuffer"};
    JSMember maxTextures{&object_,"maxTextures"};
    JSMember maxVertexTextures{&object_,"maxVertexTextures"};
    JSMember maxTextureSize{&object_,"maxTextureSize"};
    JSMember maxCubemapSize{&object_,"maxCubemapSize"};
    JSMember maxAttributes{&object_,"maxAttributes"};
    JSMember maxVertexUniforms{&object_,"maxVertexUniforms"};
    JSMember maxVaryings{&object_,"maxVaryings"};
    JSMember maxFragmentUniforms{&object_,"maxFragmentUniforms"};
    JSMember maxSamples{&object_,"maxSamples"};
    JSMember samples{&object_,"samples"};
    JSValue getMaxAnisotropy();
    JSValue getMaxPrecision(const JSValue& precision = {});
    JSValue textureFormatReadable(const JSValue& format = {});
    JSValue textureTypeReadable(const JSValue& type = {});
    static void setConsoleFunction(const JSValue& callback);
    static JSValue getConsoleFunction();
};
