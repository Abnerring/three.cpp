#pragma once
#include <string>
#include "core/JSMember.h"

JSValue nativeVarNodeConstructor();
JSValue nativeVarNodeModule();

// Port of the field rules in src/nodes/core/VarNode.js at the pinned revision. The node makes a
// shader variable out of another node, and the flags it carries say how the builder should treat
// it; four of them are worth writing down.
class VarNode {
    JSValue object_;
  public:
    explicit VarNode(const JSValue& node, const JSValue& name = nullptr, const JSValue& readOnly = false);
    VarNode(const VarNode& other) : object_(other.object_) {}
    VarNode& operator=(const VarNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeVarNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSMember node{&object_, "node"}, name{&object_, "name"}, readOnly{&object_, "readOnly"}, intent{&object_, "intent"};
    JSValue build(const std::vector<JSValue>& parameters) const;
    JSValue generate(const JSValue& builder) const;
    static constexpr bool isVarNode = true;
    static constexpr const char* kTypeName = "VarNode";

    // The five settings a fresh variable carries. Two of them are on by default and two off, and
    // the names do not all say which is which: a variable is global and requires its parents,
    // while it is neither read only nor an intent.
    static constexpr bool kGlobal = true;
    static constexpr bool kParents = true;
    static constexpr bool kReadOnly = false;
    static constexpr bool kIntent = false;

    // The name is absent by default, so one is generated rather than required.
    static bool hasNameByDefault();

    // Whether the node is treated as an intent, which is to say it marks a place where a value is
    // to be written rather than being built itself. The answer is kept in the builder's own record
    // for the node rather than in the node, and the flag here is the request; so the two can
    // disagree until the builder has looked at it.
    static bool intentRequested(bool flag);
    static bool intentComesFromTheBuilder();

    // What setting the intent flag answers, which is the node itself so that the call can be
    // chained.
    static bool setterReturnsTheNode();

    static bool isCacheable();
};
