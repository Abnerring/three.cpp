#pragma once
#include <string>

// Port of the rules in src/nodes/core/SubBuildNode.js at the pinned revision. The node builds a
// child under a named sub-build, which the builder brackets with an add and a remove.
class SubBuildNode {
  public:
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
