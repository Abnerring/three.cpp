#pragma once
#include "core/JSValue.h"
JSValue nativeSplitNodeConstructor();

#include <string>

// Port of the rules in src/nodes/utils/SplitNode.js at the pinned revision. The node reads some
// components out of a value, and three of its rules do not follow from the name of the operation.
class SplitNode {
    JSValue object_;
  public:
    explicit SplitNode(const JSValue& node = {}, const JSValue& components = {}) : object_(nativeSplitNodeConstructor().construct({node, components})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeSplitNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }

    static constexpr const char* kTypeName = "SplitNode";

    // The width a set of names needs. It is not the number of names: each name is looked up and
    // the answer is the largest position plus one, so asking for the fourth alone needs four
    // places and asking for the first and third needs three rather than two. A name that is not
    // a component contributes nothing.
    static int vectorLength(const std::string& components);

    // Whether reading these names from a value of this width is a no-op. It is when the names
    // are as many as the value is wide and are the first that many, in order: reading the first
    // two names from a two part value gives the value back, so no swizzle is emitted. Any other
    // arrangement is written out even if it names the same number.
    static bool swizzleIsIdentity(int nodeTypeLength, const std::string& components);

    // Whether the component names are read at all. A value that is one part wide ignores them,
    // so asking a float for its second component gives the float rather than an error or a
    // repetition.
    static bool componentsAreRead(int nodeTypeLength);

    // Whether the node may be cached. It may not, as with most of its neighbours.
    static bool isCacheable();
};
