#include "nodes/core/NodeBuilderHash.h"

bool NodeBuilderHash::recordIsAPlainObject() {
    return true;
}

std::optional<std::uint64_t> NodeBuilderHash::afterSetting(std::optional<std::uint64_t> existing,
                                                           std::uint64_t newNode) {
    // No check and no refusal: the new one wins.
    (void)existing;
    return newNode;
}

bool NodeBuilderHash::absentAnswersNothing(bool found) {
    return !found;
}

bool NodeBuilderHash::hashIsComputedAtAddTime() {
    return true;
}

bool NodeBuilderHash::addingTheSameNodeTwiceChangesNothing(bool alreadyPresent) {
    return alreadyPresent;
}
