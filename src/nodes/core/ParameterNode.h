#pragma once
#include <cstdint>
#include <string>

// Port of the rules in src/nodes/core/ParameterNode.js at the pinned revision. The node names a
// shader parameter, which is a property with a type and a name.
class ParameterNode {
  public:
    static constexpr const char* kTypeName = "ParameterNode";
    static constexpr bool isParameterNode = true;

    // The type a member takes when the node's own type has no structure behind it. The member is
    // reported through an error and then answered as a float, which is the same default the
    // structure family's member node uses in the same position, so the two families agree here
    // even though they disagree about the storage element case taken a few rounds earlier.
    static constexpr const char* kMissingStructureMemberType = "float";

    static std::string memberType(bool hasStruct, const std::string& structAnswer);

    // The message printed when there is no structure.
    static std::string missingStructureMessage(const std::string& name, const std::string& type);

    // The hash the node is known by, which is its identifier turned into text rather than kept
    // as a number; two nodes with different identifiers therefore differ, and the text is what a
    // caller compares.
    static std::string hash(std::uint64_t id);

    // What generating the node produces. It is the name as given, with no check that there is
    // one, so a node made without a name produces the text of the absent value.
    static std::string generate(bool hasName, const std::string& name);
};
