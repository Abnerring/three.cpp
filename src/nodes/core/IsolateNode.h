#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/IsolateNode.js at the pinned revision. The node swaps the
// builder's cache for its own while its child is worked on, and four things about that are worth
// writing down.
class IsolateNode {
  public:
    static constexpr bool isIsolateNode = true;
    static constexpr const char* kTypeName = "IsolateNode";

    // The default for whether the cache is a shared parent one, which is yes.
    static constexpr bool kDefaultParent = true;

    // The three steps a swap takes, which are the same in both of the places the swap happens.
    static std::vector<std::string> swapSteps();

    // Whether the swap is protected so that the old cache is put back even when the work between
    // throws. It is not: the restore is a plain statement after the call, so an exception leaves
    // the builder pointed at this node's cache. The same shape as the sub build's bracket.
    static bool restoresOnThrow();

    // Which of the node's methods swap the cache. Both of them do, so asking the child for its
    // type changes the cache just as building it does; a reader who took the type query for a
    // read-only question would be wrong.
    static bool typeQuerySwapsCache();

    // The two functions that make such a node, and how they differ. The shorter one takes no
    // parent and leaves the default in place; the longer one is deprecated, prints a warning and
    // then makes the shorter one and sets the parent afterwards, since the shorter one has no
    // parameter for it.
    static bool deprecatedFormSetsParentAfterwards();

    // The sentence the deprecated form prints.
    static std::string deprecationWarning();

    // Whether both names are offered as methods on a node, which they are: the deprecated name
    // still chains even though it warns.
    static bool bothNamesChain();
};
