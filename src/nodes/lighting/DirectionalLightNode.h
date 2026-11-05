#pragma once

#include "core/JSValue.h"

JSValue nativeDirectionalLightNodeModule();
JSValue nativeDirectionalLightNodeBinding(const std::string& name);
JSValue nativeDirectionalLightNodeConstructor();

class DirectionalLightNode {
    JSValue object_;
  public:
    explicit DirectionalLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeDirectionalLightNodeConstructor(); }
};
