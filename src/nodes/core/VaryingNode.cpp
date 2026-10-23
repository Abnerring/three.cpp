#include "nodes/core/VaryingNode.h"

bool VaryingNode::needsInterpolationAssigned(const std::string& shaderStage) {
    // The value assigned is whether the stage is the fragment one.
    return shaderStage == "fragment";
}

bool VaryingNode::overwritesExistingFlag() {
    // The short circuit means an existing truthy value is left alone.
    return false;
}

VaryingNode::Emission VaryingNode::emissionFor(const std::string& shaderStage) {
    // The vertex stage guards on the flow block; the others guard on nothing.
    if (shaderStage == "vertex") return Emission::PerBlock;
    return Emission::OnceInTotal;
}

bool VaryingNode::setupAndAnalyzeAreTheSame() {
    return true;
}

std::string VaryingNode::hash(bool hasName, const std::string& name,
                              const std::string& ordinaryHash) {
    // The explicit name when there is one, the ordinary hash otherwise.
    if (hasName) return name;
    return ordinaryHash;
}

bool VaryingNode::isCacheable() {
    return false;
}
