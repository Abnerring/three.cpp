#include "nodes/core/MemberNode.h"

bool MemberNode::hasMember(bool structIsMember, bool structFoundItsMember, bool foundHere) {
    // A structure that is itself a member must first have found its own member.
    if (structIsMember && !structFoundItsMember) return false;
    return foundHere;
}

std::string MemberNode::memberType(bool found, const std::string& actualType) {
    // A member that is not there is a float rather than an error.
    if (!found) return kMissingMemberType;
    return actualType;
}

bool MemberNode::guardUsesOwnProperty() {
    // The guard asks about the node's own property even when a different name was passed in.
    return true;
}

std::string MemberNode::missingMemberWarning(const std::string& property) {
    return "TSL: Member \"" + property + "\" does not exist in struct.";
}

bool MemberNode::isCacheable() {
    return false;
}
