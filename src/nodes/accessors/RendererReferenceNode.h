#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeRendererReferenceNodeConstructor();
JSValue nativeRendererReferenceNodeModule();
JSValue nativeRendererReferenceNodeBinding(const std::string& name);

class RendererReferenceNode {
    JSValue object_;
  public:
    explicit RendererReferenceNode(const JSValue& property = {}, const JSValue& inputType = {}, const JSValue& renderer = nullptr)
        : object_(constructor().construct({property,inputType,renderer})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeRendererReferenceNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
