#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/accessors/ModelNode.js at the pinned revision. The node is a
// specialisation of the object node that reads a different set of measurements and, unlike it,
// takes the object from the frame each time rather than holding one of its own.
class ModelNode {
  public:
    static constexpr const char* kTypeName = "ModelNode";

    // The five things the node may measure. The source names them in the constructor's own
    // documentation, and the exported nodes are one per name.
    static const std::vector<std::string>& scopes();

    // Whether a name is one of them. The comparison is exact, so a differently cased name is not.
    static bool isScope(const std::string& name);

    // Whether the node holds its own object. It does not: the object is taken from the frame on
    // every update, so a node built once still follows whichever object the frame names. This is
    // the difference from the node it specialises, which is given an object to hold.
    static constexpr bool holdsOwnObject = false;
};
