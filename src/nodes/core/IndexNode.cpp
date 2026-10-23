#include "nodes/core/IndexNode.h"

std::vector<std::string> IndexNode::scopes() {
    return {"vertex", "instance", "subgroup", "invocationLocal", "invocationSubgroup", "draw"};
}

std::string IndexNode::builderCall(const std::string& scope) {
    if (scope == "vertex") return "getVertexIndex";
    if (scope == "instance") return "getInstanceIndex";
    if (scope == "draw") return "getDrawIndex";
    if (scope == "invocationLocal") return "getInvocationLocalIndex";
    if (scope == "invocationSubgroup") return "getInvocationSubgroupIndex";
    if (scope == "subgroup") return "getSubgroupIndex";
    // No default: the source throws here.
    return std::string();
}

bool IndexNode::isKnown(const std::string& scope) {
    return !builderCall(scope).empty();
}

std::string IndexNode::unknownScopeMessage(const std::string& scope) {
    return "THREE.IndexNode: Unknown scope: " + scope;
}

bool IndexNode::readsDirectly(const std::string& shaderStage) {
    // Two stages, not one: the compute stage reads it as the vertex stage does.
    return shaderStage == "vertex" || shaderStage == "compute";
}
