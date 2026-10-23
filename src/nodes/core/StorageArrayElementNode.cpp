#include "nodes/core/StorageArrayElementNode.h"

std::string StorageArrayElementNode::memberType(bool hasStructType,
                                                const std::string& structAnswer) {
    // The struct answers when it is there; the empty type otherwise.
    if (hasStructType) return structAnswer;
    return kMissingMemberType;
}

bool StorageArrayElementNode::familiesAgreeOnMissingMember() {
    // The structure family gives a float where this one gives the empty type.
    return std::string(kMissingMemberType) == std::string(kMemberNodeMissingType);
}

bool StorageArrayElementNode::formats(bool isAssignContext) {
    // An assignment needs the thing being written, so no formatting.
    return !isAssignContext;
}

bool StorageArrayElementNode::usesPboFallback(bool storageBufferAvailable, bool isPbo,
                                              bool isAssignContext,
                                              bool valueIsInstancedBufferAttribute,
                                              bool isComputeStage) {
    // Only when the buffer is unavailable, the node asked for the fallback, this is not an
    // assignment, and either the value is an instanced attribute or the stage is not a compute
    // one.
    if (storageBufferAvailable) return false;
    if (!isPbo) return false;
    if (isAssignContext) return false;
    const bool byValue = valueIsInstancedBufferAttribute;
    const bool byStage = !isComputeStage;
    return byValue || byStage;
}
