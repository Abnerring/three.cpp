#pragma once
#include "core/JSValue.h"
JSValue nativeLightingContextNodeConstructor();
JSValue nativeLightingContextNodeModule();
JSValue nativeLightingContextNodeBinding(const std::string& name);
class LightingContextNode {
    JSValue object_;
  public:
    explicit LightingContextNode(const JSValue& lightsNode = {}, const JSValue& lightingModel = {}, const JSValue& materialLightings = {}, const JSValue& backdropNode = {}, const JSValue& backdropAlphaNode = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeLightingContextNodeBinding("default"); }
};
