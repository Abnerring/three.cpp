#pragma once

#include "core/JSValue.h"

// Native module for src/nodes/functions/ShadowMaskModel.js at the pinned revision.
JSValue nativeShadowMaskModelModule();
JSValue nativeShadowMaskModelBinding(const std::string& name);
JSValue nativeShadowMaskModelConstructor();

class ShadowMaskModel {
    JSValue object_;
  public:
    ShadowMaskModel();
    explicit ShadowMaskModel(const JSValue& object) : object_(object) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeShadowMaskModelConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
