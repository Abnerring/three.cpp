#pragma once
#include <string>
#include "core/JSValue.h"

JSValue nativeSubBuildNodeConstructor();
JSValue nativeSubBuildNodeModule();
JSValue nativeSubBuildNodeBinding(const std::string& name);

// Port of the rules in src/nodes/core/SubBuildNode.js at the pinned revision. The node builds a
// child under a named sub-build, which the builder brackets with an add and a remove.
class SubBuildNode {
    JSValue object_;
  public:
    SubBuildNode(const JSValue& node = {}, const JSValue& name = {}, const JSValue& nodeType = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeSubBuildNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue generateNodeType(const JSValue& builder) const;
    JSValue build(const JSValue& builder, const std::vector<JSValue>& params = {}) const;
    static constexpr const char* kTypeName = "SubBuild";
    static constexpr bool isSubBuildNode = true;

    // Whether the add and remove pair runs while the type is worked out. It does not when the
    // type was given when the node was made: the early return leaves without touching the
    // bracket, so a caller that supplies a type causes one fewer pair of builder calls than one
    // that does not.
    static bool bracketsTheTypeQuery(bool typeWasGiven);

    // Whether the pair runs while the child is built, which it always does.
    static bool bracketsTheBuild();

    // Whether the two are balanced by a construct that guarantees the second even when the first
    // succeeded and the work between them throws. There is none in the source: the remove is a
    // plain statement after the build, so an exception would leave the bracket open.
    static bool balancedOnThrow();

    static bool isCacheable();
};
