#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeReferenceElementNodeConstructor();
JSValue nativeReferenceElementNodeModule();
JSValue nativeReferenceElementNodeBinding(const std::string& name);

class ReferenceElementNode {
    JSValue object_;
  public:
    static constexpr bool isReferenceElementNode = true;
    static constexpr const char* kTypeName = "ReferenceElementNode";
    explicit ReferenceElementNode(const JSValue& referenceNode = {}, const JSValue& indexNode = {})
        : object_(constructor().construct({referenceNode,indexNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeReferenceElementNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue generateNodeType() const { return object_.get("generateNodeType").call(object_); }
    static bool needsFormatting(const std::string& arrayType, const std::string& elementType);
};
