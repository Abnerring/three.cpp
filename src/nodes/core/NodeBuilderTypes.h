#pragma once
#include <string>

// Port of the type arithmetic in src/nodes/core/NodeBuilder.js at the pinned revision. Four of its
// answers are not what the names suggest, and one of them is a collision between two types of the
// same size.
class NodeBuilderTypes {
  public:
    // How many scalars a type holds. A vector says so in its name, the plain scalars are one, and
    // a matrix is the square of its size. A type that is none of those answers nought, which is
    // neither one nor an error: a caller that multiplied by this would silently get nothing rather
    // than being told the type was unknown.
    static int typeLength(const std::string& type);

    // What kind of scalar a type is made of. A plain type answers itself; a vector or matrix
    // answers the kind its prefix names, with no prefix meaning a float. A type that is neither
    // answers nothing at all, which is the one place here that distinguishes unknown from a
    // default.
    static std::string componentType(const std::string& type);

    // A type of a given length. One is the component type itself; anything else starts from the
    // vector of that length and gains a prefix unless the component is a float, and the prefix is
    // the FIRST character of the component type rather than a word.
    static std::string typeFromLength(int length, const std::string& componentType);

    // Whether a type is one of the matrix types the length table has a special case for. A two by
    // two matrix holds four scalars, which is the same number a four part vector holds, so the
    // length alone cannot say which is meant and the matrix test has to come first; the tests
    // record the collision rather than only the answers.
    static bool isMatrix(const std::string& type);

    // Whether a buffer attribute's scalar kind can be read from its array. It cannot when the
    // attribute is a half float one or when it is normalised, in both of which cases the kind is
    // left unknown rather than guessed from an array that does not describe the values as they
    // are seen.
    static bool scalarKindIsReadable(bool isHalfFloat, bool isNormalised);

    // Whether a geometry has an attribute. A builder without a geometry answers nothing rather
    // than failing, so a caller asking before a geometry is attached gets a negative answer and
    // not an exception.
    static bool hasAttribute(bool hasGeometry, bool attributeIsPresent);
};
