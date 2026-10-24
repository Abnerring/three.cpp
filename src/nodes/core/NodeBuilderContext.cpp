#include "nodes/core/NodeBuilderContext.h"

std::map<std::string, int> NodeBuilderContext::merged(
    const std::map<std::string, int>& current, const std::map<std::string, int>& added) {
    // The added entries are spread over the current ones, so they win where they overlap.
    std::map<std::string, int> out = current;
    for (const auto& pair : added) out[pair.first] = pair.second;
    return out;
}

bool NodeBuilderContext::returnsThePreviousContext() {
    return true;
}

std::vector<std::string> NodeBuilderContext::entriesRemovedForSharing() {
    // Exactly two entries, both belonging to one material rather than to the shader construction.
    return {"material", "getUV"};
}

bool NodeBuilderContext::isRemovedForSharing(const std::string& key) {
    return key == "material" || key == "getUV";
}

NodeBuilderContext::StackStep NodeBuilderContext::addStack() {
    StackStep step;
    step.makesAChild = true;
    step.savesThePrevious = true;
    step.becomesCurrent = true;
    return step;
}

bool NodeBuilderContext::backStampsEveryNode() {
    return true;
}

int NodeBuilderContext::nestingRecordsKept() {
    return 2;
}
