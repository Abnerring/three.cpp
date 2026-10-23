#include "nodes/core/ReferenceElementNode.h"

bool ReferenceElementNode::needsFormatting(const std::string& arrayType,
                                           const std::string& elementType) {
    // The snippet is always handed to the formatter by the source, whether or not the two types
    // differ; what this answers is when the formatting would actually change anything.
    return arrayType != elementType;
}
