#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeStorageTextureNodeConstructor();
JSValue nativeStorageTextureNodeModule();
JSValue nativeStorageTextureNodeBinding(const std::string& name);

class StorageTextureNode {
    JSValue object_;
public:
    explicit StorageTextureNode(const JSValue& value = {}, const JSValue& uvNode = nullptr,
                                const JSValue& storeNode = nullptr)
        : object_(nativeStorageTextureNodeConstructor().construct({value, uvNode, storeNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeStorageTextureNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
