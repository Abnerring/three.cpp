#pragma once
#include <string>

// Port of the rules in src/nodes/utils/StorageArrayElementNode.js at the pinned revision. The
// node reads one element of a storage buffer, and three of its rules are worth writing down, one
// of them because a sibling module answers the same question differently.
class StorageArrayElementNode {
  public:
    static constexpr const char* kTypeName = "StorageArrayElementNode";

    // The type of a member. When the buffer carries a struct type the question is passed to it;
    // when it does not the answer is the empty type, which is not what the member node of the
    // structure family answers in the same position. That module gives a float, so the two
    // families disagree about what a missing member is, and a test records both answers.
    static constexpr const char* kMissingMemberType = "void";

    static std::string memberType(bool hasStructType, const std::string& structAnswer);

    // The type the member node of the structure family gives for the same question, which is a
    // float rather than the empty type.
    static constexpr const char* kMemberNodeMissingType = "float";

    // Whether the two families agree about a missing member, which they do not.
    static bool familiesAgreeOnMissingMember();

    // Whether a snippet is formatted. It is, unless the node is being assigned to: an assignment
    // needs the thing being written rather than a value of a type, so the formatting that would
    // convert one type to another is left out.
    static bool formats(bool isAssignContext);

    // Whether the storage buffer path is taken. When the backend cannot offer a storage buffer,
    // the node falls back to drawing into a texture, and the fallback is used for a particular
    // mix of conditions rather than always.
    static bool usesPboFallback(bool storageBufferAvailable, bool isPbo, bool isAssignContext,
                                bool valueIsInstancedBufferAttribute, bool isComputeStage);
};
