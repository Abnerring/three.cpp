#include "nodes/core/InputNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("InputNode derives its type from its value when none was given") {
    // The given type wins when there is one.
    REQUIRE(InputNode::nodeType(true, "vec3", "float") == "vec3");
    // Without one the value's type is used, so a node made with a number and no type reports a
    // number's type rather than nothing.
    REQUIRE(InputNode::nodeType(false, "vec3", "float") == "float");
    REQUIRE(InputNode::nodeType(false, "vec3", "float") != "vec3");
    REQUIRE(InputNode::isInputNode);
}

TEST_CASE("InputNode's input type defaults to its node type") {
    // Without a derived answer the two are the same.
    REQUIRE(InputNode::inputType(false, "vec4", "texture") == "vec4");
    // A derived class may answer otherwise, and the texture case is the example: the input is a
    // texture while the node's value is a vector of four parts.
    REQUIRE(InputNode::inputType(true, "vec4", "texture") == "texture");
    REQUIRE(InputNode::inputType(true, "vec4", "texture") != InputNode::inputType(false, "vec4", "texture"));
    REQUIRE(std::string(InputNode::kTypeName) == "InputNode");
}

TEST_CASE("InputNode serialises an array buffer as text and does not turn it back") {
    // The buffer becomes text when written out.
    REQUIRE(InputNode::serialisesArrayBufferAsText());
    // And it is not turned back when read in: the text is not an array, so it is taken as it
    // stands. The round trip therefore gives the text rather than the buffer.
    REQUIRE_FALSE(InputNode::deserialisesTextBackToArrayBuffer());
    REQUIRE(InputNode::serialisesArrayBufferAsText() !=
            InputNode::deserialisesTextBackToArrayBuffer());
    // Only an array is reconstructed from its parts, which is what the text fails to be.
    REQUIRE(InputNode::reconstructsFromParts(true));
    REQUIRE_FALSE(InputNode::reconstructsFromParts(false));
}

TEST_CASE("InputNode keeps a precision only when the stored one is truthy") {
    // A stored precision that is a value is kept.
    REQUIRE(InputNode::precisionIsSetAfterDeserialising(true, true));
    // One that is falsy falls back to nothing, which is what the short circuiting or does; a test
    // against the absence alone would keep it.
    REQUIRE_FALSE(InputNode::precisionIsSetAfterDeserialising(true, false));
    REQUIRE_FALSE(InputNode::precisionIsSetAfterDeserialising(false, false));
    // The two conditions differ, so the answers are not symmetric.
    REQUIRE(InputNode::precisionIsSetAfterDeserialising(true, true) !=
            InputNode::precisionIsSetAfterDeserialising(true, false));
}