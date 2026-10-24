#pragma once
#include <cstdint>
#include <optional>

// Port of the sharing rules in src/nodes/core/Node.js at the pinned revision. A node asked to build
// may turn out to be another node already known under the same hash, or may be replaced by an
// override, and the two checks are not equal in standing.
class NodeShared {
  public:
    // What happens when a node is asked to share, in the order the checks run. A node already
    // known under this hash is used in place of this one, but only if it is a different node: a
    // hash that points back at the node asking is not a share, and the asking node goes on to the
    // second check rather than answering with itself through the first.
    enum class Outcome { SharedByHash, Overridden, Itself };

    static Outcome outcome(bool hashFound, bool hashNodeIsThis, bool hasOverride);

    // Whether the hash check and the override check both run. They do not: the second is only
    // reached when the first found nothing to share, so a node that has a twin under its hash is
    // never overridden, and which of the two applies depends on a lookup rather than on a
    // preference the caller expressed.
    static bool overrideIsConsideredAfterHashMatch();

    // What an override does the first time it is asked for. A record is marked before the callback
    // runs, so a call that comes back round to the same node finds the mark set and is answered
    // with the stored result instead of running the callback again; without that the callback
    // could call itself for ever.
    struct OverrideState {
        bool isOverwritten = false;
        bool runsCallback = false;
    };

    static OverrideState overrideState(bool alreadyMarked);

    // Whether the override cancels itself for the duration of the callback. It does: the callback's
    // answer has the node's own override removed, so a node used inside the callback is not
    // replaced by the very override that is building it.
    static bool cancelsOwnOverrideDuringCallback();

    // What is answered when neither check produces a node, which is the node asking.
    static bool fallsBackToItself(bool sharedNodeIsPresent);

    // How a cache key is made: from the values of the children and from a key the node gives for
    // itself, both fed to one hash. So two nodes with equal children but different custom keys are
    // different keys, and a node may make itself distinct without changing what it holds.
    static bool keyUsesBothChildrenAndCustomKey();
};
