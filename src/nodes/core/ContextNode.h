#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "core/JSValue.h"

JSValue nativeContextNodeConstructor();
JSValue nativeContextNodeModule();

// Port of the rules in src/nodes/core/ContextNode.js at the pinned revision. The node wraps
// another so that the builder works on it in a different context, and it is meant to be invisible
// in every way but that one.
class ContextNode {
  public:
    static constexpr bool isContextNode = true;
    static constexpr const char* kTypeName = "ContextNode";

    // The three questions the wrapper passes straight through to the node it wraps: a node's
    // scope, its type, and the type of one of its members. So a context node has the scope and
    // type of what it wraps rather than any of its own, which is what makes it invisible.
    static std::string scope(const std::string& wrappedScope);
    static std::string nodeType(const std::string& wrappedType);
    static std::string memberType(const std::string& wrappedMemberType);

    // How the values from several context nodes are gathered. Every one of them is collected by
    // walking the tree and then merged into a fresh object from each in turn, so a key that
    // appears twice takes its later value.
    static std::map<std::string, int> gather(
        const std::vector<std::map<std::string, int>>& gatheredInOrder);

    // When the context is actually applied. The count returned by increasing usage is compared
    // against one, which is true only on the first use, so the wrapped node is built in the
    // modified context once however many times it is asked for.
    static bool appliesContext(int usageCountAfterIncrement);

    // Whether the previous context is put back when the build between throws. It is not: the
    // restore is a plain statement after the call, the third place in this layer where a pair of
    // builder calls has nothing to close it on the way out.
    static bool restoresOnThrow();

    static bool isCacheable();
};
