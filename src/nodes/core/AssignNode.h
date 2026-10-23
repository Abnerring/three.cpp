#pragma once
#include <string>

// Port of the rules in src/nodes/core/AssignNode.js at the pinned revision. The node writes one
// node's value into another, and five of its rules do not follow from that description.
class AssignNode {
  public:
    static constexpr bool isAssignNode = true;
    static constexpr const char* kTypeName = "AssignNode";

    // The type the node reports. It is the target's type when the caller asked for something
    // other than nothing, and the empty type when the caller asked for nothing: so the same node
    // answers two different types depending on what was asked of it, which no other node in this
    // layer does.
    static std::string nodeType(const std::string& output, const std::string& targetType);

    // Whether the assignment has already been written for this build. The record is a name that
    // is set at the end of the first generate, so a second generate finds it and answers with
    // the target rather than writing the assignment twice.
    static bool alreadyAssigned(bool hasPropertyName);

    // Whether the assignment has to be written one component at a time. That is needed when the
    // backend cannot assign through a swizzle, the target is a component split of something, and
    // the components are more than one and are not the first ones in order: a target naming the
    // first two of a two part value is the value itself, so no split is needed, while one naming
    // them the other way round is.
    static bool needsSplit(bool swizzleAssignAvailable, bool targetIsSplit, int componentCount,
                           int targetLength, const std::string& components);

    // Whether the plain path adds its line to the flow. It does so only when the output asked
    // for is nothing or when the source itself is nothing; otherwise the text is returned as a
    // value without being written to the flow at all.
    static bool addsLineToFlow(const std::string& output, const std::string& sourceType);

    // What the assignment's value is, which is its target rather than the source. The record set
    // at the end of a generate is the target for the same reason.
    static constexpr bool valueIsTheTarget = true;
};
