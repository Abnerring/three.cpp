#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/ArrayNode.js at the pinned revision. The node stands for a
// list of values, and how the list is made decides two things that look independent.
class ArrayNode {
  public:
    static constexpr bool isArrayNode = true;
    static constexpr const char* kTypeName = "ArrayNode";

    // What a call to the factory means, decided by how many arguments it was given. One argument
    // is a list of values and its length is the count; two are a type and a count, with no values
    // at all. So the same function answers two quite different things depending on its arity.
    struct Shape {
        bool hasType = false;
        std::string type;
        int count = 0;
        bool hasValues = false;
    };

    static Shape fromOneArgument(const std::vector<std::uint64_t>& valueIds);
    static Shape fromTwoArguments(const std::string& type, int count);

    // The type the node reports. A type given when it was made is used; otherwise the first
    // value's type is taken, which means a node made with a type and a count has no values to
    // read and a node made from a list has no type of its own. Reading the first value when
    // there are none is what the source does rather than guarding against it.
    static std::string nodeType(bool hasType, const std::string& givenType,
                                bool hasValues, const std::string& firstValueType);

    // Whether a shape can answer its type without touching a value, which is exactly when it was
    // given one.
    static bool typeIsGiven(const Shape& shape);

    // What a node turned into an array of a given length holds. Every place is filled with the
    // same node, so the list holds one object repeated rather than distinct ones; a caller that
    // expected separate entries would find them all the same.
    static std::vector<std::uint64_t> filled(int count, std::uint64_t nodeId);
};
