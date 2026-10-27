#pragma once
#include "core/JSValue.h"
JSValue nativeArrayElementNodeConstructor();

#include <string>

// Port of the rules in src/nodes/utils/ArrayElementNode.js at the pinned revision. The node reads
// one element out of an array-like value, and two of its rules are worth writing down: what it
// does to an index that is not an integer, and where it looks a member up.
class ArrayElementNode {
    JSValue object_;
  public:
    explicit ArrayElementNode(const JSValue& node = {}, const JSValue& indexNode = {}) : object_(nativeArrayElementNodeConstructor().construct({node, indexNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeArrayElementNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }

    static constexpr const char* kTypeName = "ArrayElementNode";

    // The type the index is written as. An integer that is not a vector keeps its own type; a
    // float is quietly turned into an unsigned integer, and so is a vector. A caller passing a
    // float index therefore gets a truncation in the shader rather than an error, and one
    // passing a vector gets something that reads as an unsigned integer too.
    static std::string indexType(bool isVector, bool isInteger, const std::string& actualType);

    // Whether the index keeps the type it had, which is true only for an integer that is not a
    // vector.
    static bool indexKeepsType(bool isVector, bool isInteger);

    // The node's own type, which is the type of the elements rather than of the array.
    static std::string nodeTypeFrom(const std::string& elementType);

    // Where a member lookup is sent. The source forwards it to the array-like node itself, not
    // to the element type, so reading a member of an element asks the array for it; a test that
    // expected the lookup to follow the element would be wrong.
    enum class MemberLookup { Array, Element };

    static MemberLookup memberLookupTarget();

    // Whether the node may be cached. It may not.
    static bool isCacheable();
};
