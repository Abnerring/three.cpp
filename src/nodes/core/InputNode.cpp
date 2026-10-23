#include "nodes/core/InputNode.h"

std::string InputNode::nodeType(bool typeWasGiven, const std::string& givenType,
                                const std::string& typeFromValue) {
    // The given type when there is one, the value's otherwise.
    if (typeWasGiven) return givenType;
    return typeFromValue;
}

std::string InputNode::inputType(bool derivedOverrides, const std::string& nodeTypeValue,
                                 const std::string& derivedInputType) {
    // By default the node type; a derived class may answer otherwise.
    if (derivedOverrides) return derivedInputType;
    return nodeTypeValue;
}

bool InputNode::serialisesArrayBufferAsText() {
    return true;
}

bool InputNode::deserialisesTextBackToArrayBuffer() {
    // The text is not an array, so it is taken as it stands rather than turned back.
    return false;
}

bool InputNode::reconstructsFromParts(bool deserialisedValueIsArray) {
    return deserialisedValueIsArray;
}

bool InputNode::precisionIsSetAfterDeserialising(bool hasStoredPrecision,
                                                 bool storedPrecisionTruthy) {
    // A short circuiting or, so a stored value that is falsy gives nothing.
    return hasStoredPrecision && storedPrecisionTruthy;
}
