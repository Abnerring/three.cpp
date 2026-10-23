#pragma once
#include <string>

// Port of the rules in src/nodes/utils/MemberNode.js at the pinned revision. The node reads a
// named member out of a structure, and what it does when the name is not there is the part worth
// writing down, together with one check that guards against the wrong name.
class MemberNode {
  public:
    static constexpr const char* kTypeName = "MemberNode";

    // The type a missing member takes. It is a float, chosen silently: no error is raised and the
    // generated text becomes a constant of that type, so a shader that reads a member the
    // structure does not have still compiles and gives nought where the member would have been.
    static constexpr const char* kMissingMemberType = "float";

    // Whether a member exists, given whether the structure behind it is itself a member and
    // whether that one found its own member. The check runs up the chain, so a member of a
    // member is only there when the one before it is.
    static bool hasMember(bool structIsMember, bool structFoundItsMember, bool foundHere);

    // The type a member takes, given whether it was found.
    static std::string memberType(bool found, const std::string& actualType);

    // Which name the existence check is made against. The method that answers a type takes a
    // name and checks the existence of the node's own property instead, so a caller asking about
    // a different name gets an answer guarded by the wrong question. That is what the source
    // does, and the tests record it rather than repairing it.
    static bool guardUsesOwnProperty();

    // The sentence printed when a member is missing, which names it.
    static std::string missingMemberWarning(const std::string& property);

    // Whether the node may be cached. It may not.
    static bool isCacheable();
};
