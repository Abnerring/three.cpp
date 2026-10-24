#include "nodes/core/NodeBuilderTypes.h"

int NodeBuilderTypes::typeLength(const std::string& type) {
    // The vectors say so in their name.
    if (type.size() == 4 && type.compare(0, 3, "vec") == 0) {
        const char n = type[3];
        if (n >= '2' && n <= '4') return n - '0';
    }
    // The plain scalars are one.
    if (type == "float" || type == "bool" || type == "int" || type == "uint") return 1;
    // A matrix is the square of its size.
    if (type.find("mat2") != std::string::npos) return 4;
    if (type.find("mat3") != std::string::npos) return 9;
    if (type.find("mat4") != std::string::npos) return 16;
    // Unknown, and the answer is nought rather than a failure.
    return 0;
}

std::string NodeBuilderTypes::componentType(const std::string& type) {
    if (type == "float" || type == "bool" || type == "int" || type == "uint") return type;
    // A prefix names the kind; no prefix means a float.
    std::string prefix;
    std::string rest = type;
    if (!type.empty() && (type[0] == 'b' || type[0] == 'i' || type[0] == 'u')) {
        prefix = type.substr(0, 1);
        rest = type.substr(1);
    }
    const bool isVecOrMat = rest.compare(0, 3, "vec") == 0 || rest.compare(0, 3, "mat") == 0;
    if (!isVecOrMat) return std::string();
    if (prefix == "b") return "bool";
    if (prefix == "i") return "int";
    if (prefix == "u") return "uint";
    return "float";
}

std::string NodeBuilderTypes::typeFromLength(int length, const std::string& componentType) {
    if (length == 1) return componentType;
    // A vector of that length, unless the kind is a matrix kind.
    std::string baseType = "vec" + std::to_string(length);
    // A two by two matrix holds four scalars, the same as a four part vector.
    if (isMatrix(componentType)) {
        baseType = "mat" + std::to_string(length);
    }
    // The prefix is the first character of the kind, and a float has none.
    const std::string prefix = componentType == "float" ? std::string() : componentType.substr(0, 1);
    return prefix + baseType;
}

bool NodeBuilderTypes::isMatrix(const std::string& type) {
    return type.find("mat2") != std::string::npos;
}

bool NodeBuilderTypes::scalarKindIsReadable(bool isHalfFloat, bool isNormalised) {
    // Either one leaves the kind unknown.
    return !isHalfFloat && !isNormalised;
}

bool NodeBuilderTypes::hasAttribute(bool hasGeometry, bool attributeIsPresent) {
    // No geometry answers nothing rather than failing.
    return hasGeometry && attributeIsPresent;
}
