#pragma once
#include <string>
#include <vector>

// Port of the layout arithmetic in src/nodes/core/StructTypeNode.js at the pinned revision. The
// class works out how many four byte places a structure needs, which is not the sum of its
// members' sizes: each member is padded to its own alignment, and the whole is rounded up to the
// largest alignment any member asks for.
class StructTypeNode {
  public:
    static constexpr bool isStructTypeNode = true;
    static constexpr const char* kTypeName = "StructTypeNode";

    // The type the constructor passes up, which is the word for a structure rather than one of
    // the value types.
    static constexpr const char* kConstructorType = "struct";

    // One member: how many four byte places it takes, and how many it must start on.
    struct Member {
        int itemSize = 1;
        int alignment = 1;
    };

    static int memoryLength(const std::string& type);
    static int alignment(const std::string& type);

    // The number of four byte places a structure needs. The walk keeps the largest alignment
    // seen so far, and it updates that maximum before working out the padding for the member
    // being placed, so a member's own alignment counts towards the chunk it is padded in. Each
    // member is then moved to the next offset that is a multiple of its own alignment, and the
    // total is rounded up to a multiple of the largest alignment found.
    static int length(const std::vector<Member>& members);

    // The type of a member by name. A name that is not there answers with the empty type, which
    // is what the storage element node answers too and is not what the member node, the parameter
    // node and the attribute node answer in the same position: those three give a float. So the
    // two answers a reader might take for a family rule are both in use.
    static constexpr const char* kMissingMemberType = "void";

    static std::string memberType(const std::vector<std::string>& names,
                                  const std::vector<std::string>& types, const std::string& name);

    // Whether a member description that was given as a bare string is atomic, which it is not;
    // and whether one given as an object takes its atomic flag from the object, falling back to
    // not atomic when the flag is falsy.
    static bool atomicForBareString();
    static bool atomicForObject(bool hasFlag, bool flagTruthy);
};
