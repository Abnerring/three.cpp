#include "nodes/core/StructTypeNode.h"

int StructTypeNode::memoryLength(const std::string& type) {
    // The number of four byte places each type occupies.
    if (type == "vec2") return 2;
    if (type == "vec3") return 3;
    if (type == "vec4") return 4;
    if (type == "mat3") return 12;
    if (type == "mat4") return 16;
    if (type == "bool" || type == "int" || type == "uint") return 1;
    return 1;
}

int StructTypeNode::alignment(const std::string& type) {
    // A three part vector occupies three places but must start on four, which is what makes the
    // padding appear at all.
    if (type == "vec2") return 2;
    if (type == "vec3") return 4;
    if (type == "vec4") return 4;
    if (type == "mat3") return 4;
    if (type == "mat4") return 4;
    return 1;
}

int StructTypeNode::length(const std::vector<Member>& members) {
    int maxAlignment = 1;
    int offset = 0;
    for (const auto& member : members) {
        // The maximum is updated before the padding is worked out, so this member's own
        // alignment decides the chunk it is placed in.
        if (member.alignment > maxAlignment) maxAlignment = member.alignment;
        const int chunkOffset = offset % maxAlignment;
        const int overhang = chunkOffset % member.alignment;
        if (overhang != 0) offset += member.alignment - overhang;
        offset += member.itemSize;
    }
    // The whole is rounded up to a multiple of the largest alignment.
    const int chunks = (offset + maxAlignment - 1) / maxAlignment;
    return chunks * maxAlignment;
}

std::string StructTypeNode::memberType(const std::vector<std::string>& names,
                                       const std::vector<std::string>& types,
                                       const std::string& name) {
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name && i < types.size()) return types[i];
    }
    return kMissingMemberType;
}

bool StructTypeNode::atomicForBareString() {
    return false;
}

bool StructTypeNode::atomicForObject(bool hasFlag, bool flagTruthy) {
    // A short circuiting or, so a flag that is present but falsy gives not atomic.
    return hasFlag && flagTruthy;
}
