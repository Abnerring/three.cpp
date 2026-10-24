#pragma once
#include <string>

// Port of the type kind tests in src/nodes/core/NodeBuilder.js at the pinned revision. Three of
// them are worth writing down, and one is a regular expression whose looseness is the point.
class NodeBuilderKinds {
  public:
    // What a type is when it is not a vector. A colour becomes three parts and the four texture
    // kinds become four; every other type is itself. So the answer is about what the value means
    // rather than about how many parts it has, and a caller reading a texture node's type gets the
    // vector the texture is sampled into rather than the word texture.
    static std::string vectorType(const std::string& type);

    // The type a typed array carries, looked up by its class rather than by the array itself, so
    // two arrays of the same class answer alike and one of a class the table does not know answers
    // nothing. The lookup is a table rather than a chain of comparisons, so what is recognised is
    // whatever the table holds and nothing else.
    static std::string typeFromArrayClass(const std::string& className);

    // Whether a type is an integer kind. The test is a regular expression with no anchors, so it
    // searches rather than comparing: the alternatives are the word for an integer, the word for
    // an unsigned one, and the two vector prefixes. A name that merely contains one of those
    // matches, so a type spelled print or joint is an integer by this test even though it is not
    // one. The loose match is the source's and the tests record it rather than tightening it.
    static bool isInteger(const std::string& type);

    // Whether a boolean vector is an integer by this test, which it is not: the prefix for a
    // boolean vector is not one of the alternatives, so the test answers no even though the type
    // is a vector of a scalar kind.
    static bool booleanVectorIsInteger();
};
