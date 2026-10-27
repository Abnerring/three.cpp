#pragma once
#include "core/JSMember.h"

JSValue nativeOperatorNodeConstructor();
JSValue nativeOperatorNodeModule();
JSValue nativeOperatorNodeBinding(const std::string& name);

class OperatorNode {
    JSValue object_;
  public:
    OperatorNode(const JSValue& op, const JSValue& aNode, const JSValue& bNode = {},
                 const std::vector<JSValue>& params = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeOperatorNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember op{&object_, "op"}, aNode{&object_, "aNode"}, bNode{&object_, "bNode"};
    JSValue getOperatorMethod(const JSValue& builder, const JSValue& output) const;
    JSValue generateNodeType(const JSValue& builder, const JSValue& output = nullptr) const;
    JSValue generate(const JSValue& builder, const JSValue& output = {}) const;
    void serialize(const JSValue& data) const;
    void deserialize(const JSValue& data) const;
};
