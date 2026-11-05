#pragma once

#include "core/JSValue.h"

JSValue nativeShadowBaseNodeModule();
JSValue nativeShadowBaseNodeBinding(const std::string& name);
JSValue nativeShadowBaseNodeConstructor();
JSValue nativeShadowPositionWorld();

class ShadowBaseNode {
    JSValue object_;
  public:
    explicit ShadowBaseNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeShadowBaseNodeConstructor(); }
};
