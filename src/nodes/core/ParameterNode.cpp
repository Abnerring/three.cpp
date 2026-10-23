#include "nodes/core/ParameterNode.h"

#include <string>

std::string ParameterNode::memberType(bool hasStruct, const std::string& structAnswer) {
    // The struct answers when it is there; a float otherwise, after an error has been reported.
    if (hasStruct) return structAnswer;
    return kMissingStructureMemberType;
}

std::string ParameterNode::missingStructureMessage(const std::string& name,
                                                   const std::string& type) {
    return "TSL: Member \"" + name + "\" not found in struct \"" + type + "\".";
}

std::string ParameterNode::hash(std::uint64_t id) {
    // The identifier as text rather than as a number.
    return std::to_string(id);
}

std::string ParameterNode::generate(bool hasName, const std::string& name) {
    // No check that a name was given, so an absent one produces whatever an absent one renders
    // as rather than anything the node supplies.
    if (hasName) return name;
    return "null";
}
