#pragma once
#include <cstdint>
#include <string>

// Port of src/nodes/core/TempNode.js at the pinned revision. The module is deprecated upstream:
// it asks to be replaced by extending the node directly, and its constructor says so once per
// instance. What is left is the caching rule it exists for, which decides whether a node used
// more than once should be given a temporary variable so that it is not worked out twice.
class TempNode {
  public:
    static constexpr bool isTempNode = true;

    static constexpr const char* kTypeName = "TempNode";

    // The message the constructor emits, which names the module and what to do instead.
    static std::string deprecationWarning();

    // Whether a node should be cached. The answer is whether it has more than one dependency to
    // other nodes, which the builder counts as uses: a node used exactly once is not cached, so
    // the comparison is strictly greater than one rather than greater than or equal to two.
    static bool isCacheable(std::uint32_t usageCount);

    // Whether the node has more than one dependency, which is the question the source's
    // hasDependencies asks of the builder and which the caching rule above rests on.
    static bool hasDependencies(std::uint32_t usageCount);
};
