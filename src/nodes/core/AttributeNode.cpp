#include "nodes/core/AttributeNode.h"

std::string AttributeNode::nodeType(bool typeWasGiven, const std::string& givenType,
                                    bool hasGeometryAttribute,
                                    const std::string& typeFromAttribute) {
    // The given type when there is one; otherwise the attribute's type, or a float if the
    // geometry has no such attribute.
    if (typeWasGiven) return givenType;
    if (hasGeometryAttribute) return typeFromAttribute;
    return kMissingAttributeType;
}

std::string AttributeNode::missingAttributeWarning(const std::string& attributeName) {
    return "AttributeNode: Vertex attribute \"" + attributeName + "\" not found on geometry.";
}

AttributeNode::Form AttributeNode::formFor(const std::string& shaderStage) {
    // Only the vertex stage reads the attribute itself.
    if (shaderStage == "vertex") return Form::Attribute;
    return Form::Varying;
}

bool AttributeNode::hashIsTheName() {
    return true;
}

bool AttributeNode::isCacheable() {
    return false;
}
