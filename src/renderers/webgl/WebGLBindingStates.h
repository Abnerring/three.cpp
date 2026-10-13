#pragma once
#include "core/JSValue.h"

// three.js binding-state CPU protocol. gl supplies native commands; this class
// creates no OpenGL context. Vulkan VAO/vertex-input adaptation is separate.
class WebGLBindingStates {
    JSValue object_;
  public:
    WebGLBindingStates(const JSValue& gl = {}, const JSValue& attributes = {});
    // Runtime/wrapper lifetime remains confined to its owning thread. Entry
    // validation rejects foreign heap values before copying managed handles.
    JSValue object() const { static_cast<void>(JSWeakValue(object_));return object_; }
    operator JSValue() const { return object(); }
    JSValue setup(const JSValue& object = {}, const JSValue& material = {}, const JSValue& program = {},
                  const JSValue& geometry = {}, const JSValue& index = {});
    JSValue reset();
    JSValue resetDefaultState();
    JSValue dispose();
    JSValue releaseStatesOfGeometry(const JSValue& geometry = {});
    JSValue releaseStatesOfObject(const JSValue& object = {});
    JSValue releaseStatesOfProgram(const JSValue& program = {});
    JSValue initAttributes();
    JSValue enableAttribute(const JSValue& attribute = {});
    JSValue disableUnusedAttributes();
};
