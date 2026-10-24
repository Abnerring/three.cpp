#include "nodes/core/NodeBuilderFormat.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeBuilder starts a constant from a value for some types only") {
    // The scalars start at nought, the boolean at false.
    REQUIRE(NodeBuilderFormat::startsWithAValue("float"));
    REQUIRE(NodeBuilderFormat::startsWithAValue("int"));
    REQUIRE(NodeBuilderFormat::startsWithAValue("uint"));
    REQUIRE(NodeBuilderFormat::startsWithAValue("bool"));
    REQUIRE(NodeBuilderFormat::startsWithAValue("color"));
    // And every spelling of every vector length is listed, four to a length.
    for (const char* name : {"vec2", "uvec2", "ivec2", "bvec2", "vec3", "uvec3", "ivec3", "bvec3",
                            "vec4", "uvec4", "ivec4", "bvec4"}) {
        REQUIRE(NodeBuilderFormat::startsWithAValue(name));
    }
    REQUIRE(NodeBuilderFormat::spellingsPerVectorLength() == 4);
    // A matrix is not among them, so it keeps nothing and the code after would read through it.
    REQUIRE_FALSE(NodeBuilderFormat::startsWithAValue("mat4"));
    REQUIRE_FALSE(NodeBuilderFormat::startsWithAValue("mat2"));
    REQUIRE_FALSE(NodeBuilderFormat::startsWithAValue(""));
}

TEST_CASE("NodeBuilder clamps a negative unsigned constant to nought") {
    // A value that is not negative is written with a u after it.
    REQUIRE(NodeBuilderFormat::unsignedText(0) == "0u");
    REQUIRE(NodeBuilderFormat::unsignedText(7) == "7u");
    REQUIRE(NodeBuilderFormat::unsignedText(1234) == "1234u");
    // A negative one becomes zero rather than its own size, which is a clamp and not an absolute
    // value; the same choice the constant node makes.
    REQUIRE(NodeBuilderFormat::unsignedText(-1) == "0u");
    REQUIRE(NodeBuilderFormat::unsignedText(-7) == "0u");
    REQUIRE(NodeBuilderFormat::unsignedText(-7) != "7u");
    REQUIRE_FALSE(NodeBuilderFormat::negativeUnsignedKeepsItsSize());
}

TEST_CASE("NodeBuilder lets a snippet through unchanged in three cases") {
    // The two types agree: nothing to do.
    REQUIRE(NodeBuilderFormat::passesThrough("vec3", false, "vec3", true, false));
    // Nothing was asked for: nothing to do, and the caller is not told that none was possible.
    REQUIRE(NodeBuilderFormat::passesThrough("vec3", true, "", false, false));
    // What was asked for is a reference rather than a value.
    REQUIRE(NodeBuilderFormat::passesThrough("vec3", false, "ref", false, true));
    // With none of the three the snippet is converted.
    REQUIRE_FALSE(NodeBuilderFormat::passesThrough("vec3", false, "vec4", false, false));
    REQUIRE(NodeBuilderFormat::passesThrough("vec3", false, "vec4", false, true) !=
            NodeBuilderFormat::passesThrough("vec3", false, "vec4", false, false));
}

TEST_CASE("NodeBuilder ignores a value longer than four parts") {
    // The source leaves a note saying to ignore it for now and does nothing further, so a matrix
    // being formatted into something else is not converted and the snippet comes back as it came.
    REQUIRE(NodeBuilderFormat::longValuesAreIgnored());
    // A four by four matrix is written from three rows of three.
    REQUIRE(NodeBuilderFormat::sixteenToNineUsesThreeRows());
}