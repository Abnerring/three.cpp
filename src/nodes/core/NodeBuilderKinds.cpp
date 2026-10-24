#include "nodes/core/NodeBuilderKinds.h"

std::string NodeBuilderKinds::vectorType(const std::string& type) {
    // A colour is three parts.
    if (type == "color") return "vec3";
    // The four texture kinds are sampled into four parts.
    if (type == "texture" || type == "cubeTexture" || type == "storageTexture" ||
        type == "texture3D") {
        return "vec4";
    }
    return type;
}

std::string NodeBuilderKinds::typeFromArrayClass(const std::string& className) {
    // A table lookup by class, so only what the table holds is recognised.
    if (className == "Float32Array") return "float";
    if (className == "Int32Array") return "int";
    if (className == "Uint32Array") return "uint";
    if (className == "Uint16Array") return "uint";
    if (className == "Uint8Array") return "uint";
    if (className == "Int16Array") return "int";
    if (className == "Int8Array") return "int";
    return std::string();
}

bool NodeBuilderKinds::isInteger(const std::string& type) {
    // Unanchored, so a name that merely contains one of the alternatives matches.
    if (type.find("int") != std::string::npos) return true;
    if (type.find("uint") != std::string::npos) return true;
    if (type.find("ivec") != std::string::npos) return true;
    if (type.find("uvec") != std::string::npos) return true;
    return false;
}

bool NodeBuilderKinds::booleanVectorIsInteger() {
    // The boolean prefix is not one of the alternatives.
    return false;
}
