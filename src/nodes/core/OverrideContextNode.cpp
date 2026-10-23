#include "nodes/core/OverrideContextNode.h"

OverrideContextNode::Stored OverrideContextNode::storedKind(bool hasValue, bool valueIsFunction) {
    if (!hasValue) return Stored::Nothing;
    if (valueIsFunction) return Stored::Function;
    return Stored::Wrapper;
}

bool OverrideContextNode::singleAsksWhetherThereIsAnything() {
    // It only asks whether the callback is a node, so a function falls through as it is.
    return false;
}

bool OverrideContextNode::multipleAsksWhetherThereIsAnything() {
    // The nested form asks about nothing first.
    return true;
}

std::vector<std::string> OverrideContextNode::freeFunctionArgumentOrder() {
    return {"targetNode", "callback", "flowNode"};
}

std::vector<std::string> OverrideContextNode::chainedArgumentOrder() {
    // The flow node comes first in the chain and is passed through last.
    return {"flowNode", "targetNode", "callback"};
}

std::uint64_t OverrideContextNode::mergedValue(
    const std::vector<std::uint64_t>& earlierThenLater) {
    // A map built from every entry in turn keeps the last value for a repeated key.
    std::uint64_t value = 0;
    for (const auto entry : earlierThenLater) value = entry;
    return value;
}

bool OverrideContextNode::gathersEveryAncestor() {
    return true;
}
