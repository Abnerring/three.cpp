#pragma once
#include <string>

// Port of the rules in src/nodes/core/StackNode.js at the pinned revision. The node collects the
// steps of a branch so that they can be written as a block, and three of its rules are worth
// writing down.
class StackNode {
  public:
    static constexpr bool isStackNode = true;
    static constexpr const char* kTypeName = "StackNode";

    // The three questions the stack passes to its output: the element type, the node type, and
    // the type of one of its members. All three answer with the empty type when there is no
    // output, so a stack that was never given one is void in every sense rather than being
    // refused.
    static std::string elementType(bool hasOutput, const std::string& outputElementType);
    static std::string nodeType(bool hasOutput, const std::string& outputType);
    static std::string memberType(bool hasOutput, const std::string& outputMemberType);

    // Whether the result of building the output is kept. It is kept unless the build is at the
    // third stage and the output's type is the empty one, so only that one pairing throws the
    // result away; at any other stage the result comes back whatever its type, and at the third
    // stage a typed output still comes back.
    static bool keepsResult(const std::string& buildStage, const std::string& outputType);

    // What building a stack with no output does, which is to build the node itself rather than
    // its output.
    static bool fallsBackToItself(bool hasOutput);

    // Whether the current stack is put back when the build between throws. It is not, the restore
    // being a plain statement at the end, which is the fourth place in this layer where a pair of
    // builder calls has nothing to close it on the way out.
    static bool restoresOnThrow();

    static bool isCacheable();
};
