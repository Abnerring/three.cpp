#pragma once
#include <string>

// Port of the rules in src/nodes/accessors/ReferenceElementNode.js at the pinned revision. The
// node refers to one element of an array-like reference, so its own type is the element's rather
// than the array's. Two things about how it works that out are worth writing down.
class ReferenceElementNode {
  public:
    static constexpr bool isReferenceElementNode = true;
    static constexpr const char* kTypeName = "ReferenceElementNode";

    explicit ReferenceElementNode(std::string uniformType)
        : uniformType_(std::move(uniformType)) {}

    // The type the node reports. It is taken from the reference node's uniform type rather than
    // asked of the builder, so the method that answers it takes no builder at all: whatever the
    // builder would have said is not consulted.
    const std::string& generateNodeType() const { return uniformType_; }

    // Whether the two types differ, which decides whether the snippet the parent generates has
    // to be converted before it is used. The array type comes from the reference node and the
    // element type from this node, so the answer says whether those two disagree.
    static bool needsFormatting(const std::string& arrayType, const std::string& elementType);

  private:
    std::string uniformType_;
};
