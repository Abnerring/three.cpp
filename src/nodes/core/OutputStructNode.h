#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/OutputStructNode.js at the pinned revision. The node writes
// several values out at once, and three of its rules are worth writing down.
class OutputStructNode {
  public:
    static constexpr bool isOutputStructNode = true;
    static constexpr const char* kTypeName = "OutputStructNode";

    // The type the node reports. It is one fixed word whatever the members are, so two nodes with
    // different members report the same type; the shape is carried by the structure the builder
    // makes, not by this answer.
    static constexpr const char* kNodeType = "OutputType";

    // The name of a member. The names are positional, so the first is m nought whatever its
    // value's type, rather than taking a name from the value.
    static std::string memberName(int index);

    // The prefix written before a member's name. An empty property name gives no prefix at all,
    // not a bare dot, so a line is written as the name alone rather than with a leading dot.
    static std::string structPrefix(const std::string& propertyName);

    // The line written for one member.
    static std::string memberLine(const std::string& propertyName, int index,
                                  const std::string& snippet);

    // What generating the node answers, which is the property name rather than a constructor of
    // the members.
    static std::string generateAnswer(const std::string& propertyName);

    // Whether the layout is worked out once and kept, which it is: the check is against the
    // record being absent.
    static bool layoutIsCached();

    static bool isCacheable();
};
