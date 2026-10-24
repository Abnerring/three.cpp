#include "nodes/core/NodeShared.h"

NodeShared::Outcome NodeShared::outcome(bool hashFound, bool hashNodeIsThis, bool hasOverride) {
    // A hash that leads to a different node is a share; one that leads back here is not.
    if (hashFound && !hashNodeIsThis) return Outcome::SharedByHash;
    // Only then is the override considered.
    if (hasOverride) return Outcome::Overridden;
    return Outcome::Itself;
}

bool NodeShared::overrideIsConsideredAfterHashMatch() {
    // The second check is in an else branch, so a hash match skips it.
    return false;
}

NodeShared::OverrideState NodeShared::overrideState(bool alreadyMarked) {
    OverrideState out;
    // The flag reports whether the node was already marked before this call, so the first call
    // sees it clear and a re-entry sees it set; it is not the state after the call.
    out.isOverwritten = alreadyMarked;
    // The mark is set before the callback, so a re-entry finds it and does not run the callback.
    out.runsCallback = !alreadyMarked;
    return out;
}

bool NodeShared::cancelsOwnOverrideDuringCallback() {
    return true;
}

bool NodeShared::fallsBackToItself(bool sharedNodeIsPresent) {
    return !sharedNodeIsPresent;
}

bool NodeShared::keyUsesBothChildrenAndCustomKey() {
    return true;
}
