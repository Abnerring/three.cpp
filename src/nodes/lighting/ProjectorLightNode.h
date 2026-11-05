#pragma once

#include "core/JSValue.h"

JSValue nativeProjectorLightNodeModule();
JSValue nativeProjectorLightNodeBinding(const std::string& name);
JSValue nativeProjectorLightNodeConstructor();

class ProjectorLightNode {
    JSValue object_;
  public:
    explicit ProjectorLightNode(const JSValue& light = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeProjectorLightNodeConstructor(); }
};
