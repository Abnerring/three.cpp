#include "nodes/core/ConstNode.h"

bool ConstNode::isNumericType(const std::string& typeName) {
    // The source's pattern is /float|u?int/, which matches "float", "int" and "uint", and does so
    // wherever they appear rather than only at the start. A name holding one of them therefore
    // counts, which is a wider test than comparing whole names.
    if (typeName.find("float") != std::string::npos) return true;
    if (typeName.find("int") != std::string::npos) return true;
    if (typeName.find("uint") != std::string::npos) return true;
    return false;
}

bool ConstNode::writesDirectly(const std::string& typeName, const std::string& outputType) {
    // Both sides must be numeric; a numeric constant going to a vector is converted instead.
    return isNumericType(typeName) && isNumericType(outputType);
}

double ConstNode::numericValue(const std::string& typeName, double value) {
    // An integer loses its fraction towards zero. An unsigned one that is negative becomes zero
    // before the cut, so the two differ for every negative value.
    if (typeName == "int") return std::trunc(value);
    if (typeName == "uint") return value >= 0 ? std::trunc(value) : 0.0;
    // Any other type keeps the value as it is, since only the two named are handled here.
    return value;
}

bool ConstNode::isWeak(bool hasValue, bool valueIsTrue) {
    // A document that says nothing leaves the constant fixed, and one that says anything other
    // than exactly true leaves it fixed too.
    return hasValue && valueIsTrue;
}
