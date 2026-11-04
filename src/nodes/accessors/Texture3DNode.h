#pragma once

#include "core/JSValue.h"

#include <string>

JSValue nativeTexture3DNodeConstructor();
JSValue nativeTexture3DNodeModule();
JSValue nativeTexture3DNodeBinding(const std::string& name);

class Texture3DNode {
    JSValue object_;

public:
    Texture3DNode(const JSValue& value = {}, const JSValue& uvNode = nullptr,
                  const JSValue& levelNode = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeTexture3DNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
