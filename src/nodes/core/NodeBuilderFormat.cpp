#include "nodes/core/NodeBuilderFormat.h"

bool NodeBuilderFormat::startsWithAValue(const std::string& type) {
    // The scalars, the boolean, the colour and every spelling of every vector length.
    if (type == "float" || type == "int" || type == "uint") return true;
    if (type == "bool") return true;
    if (type == "color") return true;
    static const char* kVectorTypes[] = {"vec2",  "uvec2", "ivec2", "bvec2", "vec3", "uvec3",
                                        "ivec3", "bvec3", "vec4",  "uvec4", "ivec4", "bvec4"};
    for (const char* known : kVectorTypes) {
        if (type == known) return true;
    }
    // A matrix, or anything else, keeps nothing at all.
    return false;
}

int NodeBuilderFormat::spellingsPerVectorLength() {
    // The plain, unsigned, signed and boolean spellings.
    return 4;
}

std::string NodeBuilderFormat::unsignedText(long long value) {
    // A clamp to nought rather than an absolute value.
    if (value < 0) return "0u";
    return std::to_string(value) + "u";
}

bool NodeBuilderFormat::negativeUnsignedKeepsItsSize() {
    return false;
}

bool NodeBuilderFormat::passesThrough(const std::string& fromType, bool toTypeIsNull,
                                      const std::string& toType, bool vectorTypesAgree,
                                      bool toTypeIsReference) {
    if (vectorTypesAgree) return true;
    if (toTypeIsNull) return true;
    if (toTypeIsReference) return true;
    (void)fromType;
    (void)toType;
    return false;
}

bool NodeBuilderFormat::longValuesAreIgnored() {
    return true;
}

bool NodeBuilderFormat::sixteenToNineUsesThreeRows() {
    return true;
}
