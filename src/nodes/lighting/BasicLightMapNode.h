#pragma once

#include "core/JSValue.h"

// Native counterpart of src/nodes/lighting/BasicLightMapNode.js.
JSValue nativeBasicLightMapNodeModule();
JSValue nativeBasicLightMapNodeBinding(const std::string& name);
JSValue nativeBasicLightMapNodeConstructor();

class BasicLightMapNode {
    JSValue object_;
  public:
    explicit BasicLightMapNode(const JSValue& lightMapNode = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeBasicLightMapNodeConstructor(); }
};
