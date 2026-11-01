#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader tells a part from a primitive") {
    // The two words that make a part.
    REQUIRE(LDrawLoader::isPartType("Part"));
    REQUIRE(LDrawLoader::isPartType("Unofficial_Part"));
    // The case matters, so a lower case spelling is not a part.
    REQUIRE_FALSE(LDrawLoader::isPartType("part"));
    REQUIRE_FALSE(LDrawLoader::isPartType("PART"));
    // Nor is a plural or a longer name.
    REQUIRE_FALSE(LDrawLoader::isPartType("Parts"));
    REQUIRE_FALSE(LDrawLoader::isPartType("Subpart"));
    // Nor is a bare word that merely contains it.
    REQUIRE_FALSE(LDrawLoader::isPartType("Parts_Primitive"));
    REQUIRE_FALSE(LDrawLoader::isPartType(""));
}

TEST_CASE("LDrawLoader finds a primitive anywhere in the type") {
    // The word is looked for inside the declaration.
    REQUIRE(LDrawLoader::isPrimitiveType("Primitive"));
    REQUIRE(LDrawLoader::isPrimitiveType("Unofficial_Primitive"));
    REQUIRE(LDrawLoader::isPrimitiveType("Primitives"));
    // The case is ignored here, unlike the part test.
    REQUIRE(LDrawLoader::isPrimitiveType("PRIMITIVE"));
    REQUIRE(LDrawLoader::isPrimitiveType("primitive"));
    REQUIRE(LDrawLoader::isPrimitiveType("unofficial_primitive"));
}

TEST_CASE("LDrawLoader still matches a subpart exactly") {
    // The subpart word is its own exact test, with its case.
    REQUIRE(LDrawLoader::isPrimitiveType("Subpart"));
    REQUIRE_FALSE(LDrawLoader::isPrimitiveType("subpart"));
    REQUIRE_FALSE(LDrawLoader::isPrimitiveType("Subparts"));
    // And it is not caught by the primitive search.
    REQUIRE_FALSE(LDrawLoader::isPartType("Subpart"));
}

TEST_CASE("LDrawLoader separates the two questions") {
    // A part is not a primitive and a primitive is not a part, so the two tests never agree.
    REQUIRE_FALSE(LDrawLoader::isPrimitiveType("Part"));
    REQUIRE_FALSE(LDrawLoader::isPartType("Primitive"));
    // A model is neither.
    REQUIRE_FALSE(LDrawLoader::isPartType("Model"));
    REQUIRE_FALSE(LDrawLoader::isPrimitiveType("Model"));
    // Nor is nothing.
    REQUIRE_FALSE(LDrawLoader::isPartType(""));
    REQUIRE_FALSE(LDrawLoader::isPrimitiveType(""));
}
