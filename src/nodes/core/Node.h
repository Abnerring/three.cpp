#pragma once
#include "core/JSMember.h"

// Live native implementation of the pinned Node.js module. Fields, methods,
// children and builder interactions share the same managed object graph.
JSValue nativeNodeConstructor();
class Node {
    JSValue object_;
  public:
    explicit Node(const JSValue& nodeType = nullptr);
    Node(const Node& other) : object_(other.object_) {}
    Node& operator=(const Node& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember nodeType{&object_, "nodeType"}, updateType{&object_, "updateType"};
    JSMember updateBeforeType{&object_, "updateBeforeType"}, updateAfterType{&object_, "updateAfterType"};
    JSMember version{&object_, "version"}, name{&object_, "name"}, global{&object_, "global"};
    JSMember parents{&object_, "parents"}, isNode{&object_, "isNode"}, id{&object_, "id"};
    JSMember stackTrace{&object_, "stackTrace"}, needsUpdate{&object_, "needsUpdate"};
    JSMember uuid{&object_, "uuid"}, type{&object_, "type"};
    JSValue build(const JSValue& builder, const JSValue& output = nullptr) const;
    JSValue getNodeType(const JSValue& builder, const JSValue& output = nullptr) const;
    JSValue getCacheKey(const JSValue& force = false, const JSValue& ignores = nullptr) const;
    JSValue toJSON(const JSValue& meta = {}) const;
    void dispose() const;
};
