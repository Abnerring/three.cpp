#include "nodes/core/ArrayElementNode.h"

bool ArrayElementNode::indexKeepsType(bool isVector, bool isInteger) {
    // Only an integer that is not a vector.
    return !isVector && isInteger;
}

std::string ArrayElementNode::indexType(bool isVector, bool isInteger,
                                        const std::string& actualType) {
    // The type itself when it is kept, an unsigned integer otherwise.
    if (indexKeepsType(isVector, isInteger)) return actualType;
    return "uint";
}

std::string ArrayElementNode::nodeTypeFrom(const std::string& elementType) {
    return elementType;
}

ArrayElementNode::MemberLookup ArrayElementNode::memberLookupTarget() {
    // The array, not the element.
    return MemberLookup::Array;
}

bool ArrayElementNode::isCacheable() {
    return false;
}
