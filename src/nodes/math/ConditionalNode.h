#pragma once
#include "core/JSMember.h"

JSValue nativeConditionalNodeConstructor();
JSValue nativeConditionalNodeModule();
JSValue nativeConditionalNodeBinding(const std::string& name);
class ConditionalNode {
    JSValue object_;
  public:
    ConditionalNode(const JSValue& condition, const JSValue& ifNode, const JSValue& elseNode = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeConditionalNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue generateNodeType(const JSValue& builder) const;
    JSValue setup(const JSValue& builder) const;
    JSValue generate(const JSValue& builder, const JSValue& output = {}) const;
};
