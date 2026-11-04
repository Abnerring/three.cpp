#pragma once

#include "core/JSValue.h"

#include <string>

JSValue nativeStorageTexture3DNodeConstructor();
JSValue nativeStorageTexture3DNodeModule();
JSValue nativeStorageTexture3DNodeBinding(const std::string& name);

class StorageTexture3DNode {
    JSValue object_;

public:
    explicit StorageTexture3DNode(const JSValue& value = {}, const JSValue& uvNode = nullptr,
                                  const JSValue& storeNode = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeStorageTexture3DNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
