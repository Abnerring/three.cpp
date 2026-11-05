#pragma once

#include "core/JSValue.h"

JSValue nativeAmbientLightNodeModule();
JSValue nativeAmbientLightNodeBinding(const std::string& name);
JSValue nativeAmbientLightNodeConstructor();

class AmbientLightNode {
    JSValue object_;
  public:
    explicit AmbientLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeAmbientLightNodeConstructor(); }
};
