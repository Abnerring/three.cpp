#pragma once
#include <string>

// Port of src/nodes/accessors/BuiltinNode.js at the pinned revision. The node names one of the
// shader's own variables so that a value may be written to it, which features such as clipped
// vertices need. Two things about it are worth writing down: its type is fixed at a float
// whatever it names, so the type carries no information about the variable; and it says it may
// never be cached, like the point coordinate node.
class BuiltinNode {
  public:
    static constexpr bool isBuiltinNode = true;

    // The type every instance reports. It is fixed rather than derived from the name, so a node
    // naming a four part built-in still calls itself a float.
    static constexpr const char* kNodeType = "float";

    explicit BuiltinNode(std::string name) : name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    // Whether the node may be cached. It may not, and says so plainly rather than asking.
    static bool isCacheable();

    // The shader text the node generates, which is the variable's name exactly as given: the
    // node does not check it against a list of known variables, so a name the shader does not
    // know is passed through rather than refused.
    std::string generate() const;

  private:
    // The variable name, which is the whole of what the node carries.
    std::string name_;
};
