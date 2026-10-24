#pragma once
#include <map>
#include <string>
#include <vector>

// Port of the context and stack bookkeeping in src/nodes/core/NodeBuilder.js at the pinned
// revision. The builder keeps two kinds of nesting at once, and both have an order that matters.
class NodeBuilderContext {
  public:
    // What adding context does. The new entries are spread over the old so the new win where they
    // overlap, and the call answers with the context that was in place before, which is what a
    // caller needs to put back afterwards.
    static std::map<std::string, int> merged(const std::map<std::string, int>& current,
                                             const std::map<std::string, int>& added);

    static bool returnsThePreviousContext();

    // What a context is that may be shared between materials. Two entries are removed from a copy:
    // the material and the coordinates getter, both of which belong to one material rather than to
    // the shader construction, so a shared context is the current one minus exactly those two.
    static std::vector<std::string> entriesRemovedForSharing();

    static bool isRemovedForSharing(const std::string& key);

    // What adding a stack does, in order. A new stack is made with the current one as its parent,
    // the previous current stack is saved, and the new one becomes current, so there are two
    // records to keep straight: the builder's own chain and the module's notion of the current
    // stack. Both are updated, and the saved one is what the removal restores rather than the
    // parent.
    struct StackStep {
        bool makesAChild = false;
        bool savesThePrevious = false;
        bool becomesCurrent = false;
    };

    static StackStep addStack();

    // What removing a stack does before it steps back: every node that was collected in the stack
    // is marked with it, so the record of which stack a node belonged to survives the removal
    // rather than disappearing with the stack.
    static bool backStampsEveryNode();

    // How many records the builder keeps for its nesting, which is two.
    static int nestingRecordsKept();
};
