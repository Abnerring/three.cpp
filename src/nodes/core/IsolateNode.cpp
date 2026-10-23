#include "nodes/core/IsolateNode.h"

std::vector<std::string> IsolateNode::swapSteps() {
    // Remember the old cache, set this node's, and put the old one back afterwards.
    return {"getCache", "setCache", "restoreCache"};
}

bool IsolateNode::restoresOnThrow() {
    // A plain restore after the call, with nothing to run it on the way out.
    return false;
}

bool IsolateNode::typeQuerySwapsCache() {
    // The type query does the same three steps as the build.
    return true;
}

bool IsolateNode::deprecatedFormSetsParentAfterwards() {
    return true;
}

std::string IsolateNode::deprecationWarning() {
    return "TSL: \"cache()\" has been deprecated. Use \"isolate()\" instead.";
}

bool IsolateNode::bothNamesChain() {
    // Both are registered as methods, so the deprecated name still works.
    return true;
}
