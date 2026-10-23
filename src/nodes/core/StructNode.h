#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/StructNode.js at the pinned revision. The node builds a
// value of a custom structure, and four of its rules do not follow from the example in its
// documentation.
class StructNode {
  public:
    static constexpr bool isStructNode = true;
    static constexpr const char* kTypeName = "StructNode";

    // The type the constructor passes up. It is a vector, and it is never used, because the node
    // overrides the method that reports its type and answers with the structure's instead. A
    // reader who took the constructor's argument for the node's type would be wrong.
    static constexpr const char* kConstructorType = "vec3";

    // Whether the constructor's type is ever reported, which it is not.
    static bool constructorTypeIsUsed();

    // Which values a call supplies. A call with separate arguments has them matched to the
    // member names by position, so the first argument becomes the first member whatever it was
    // meant to be; a call with a single object has it taken as it stands; and a call with
    // nothing gives nothing at all rather than an empty object.
    enum class Values { ZippedByName, TakenAsGiven, None };

    static Values valuesKind(int argumentCount, bool firstArgumentIsAnArray);

    // Which member a positional argument fills, given the member order. The names are taken in
    // the order the layout lists them, so an argument does not say which member it is for.
    static std::string memberForPosition(const std::vector<std::string>& memberOrder, int position);

    // Where the structure type is placed among the children. It is moved to the end, whatever
    // order it was in, because the code has to be generated with the type after the parts that
    // use it; the source's comment says as much.
    static bool movesStructTypeLast();

    // Whether the node may be cached. It may not.
    static bool isCacheable();
};
