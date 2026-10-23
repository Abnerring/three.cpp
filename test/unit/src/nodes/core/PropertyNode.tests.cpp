#include "nodes/core/PropertyNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("PropertyNode's output type name is a request to look the type up") {
    // The one name that is not a type is answered from the builder instead, so a property
    // declared that way takes the output's type at build time.
    REQUIRE(PropertyNode::nodeType("output", "vec4") == "vec4");
    REQUIRE(PropertyNode::nodeType("output", "float") == "float");
    // Every other name is taken as it stands.
    REQUIRE(PropertyNode::nodeType("vec3", "vec4") == "vec3");
    REQUIRE(PropertyNode::nodeType("vec3", "vec4") != PropertyNode::nodeType("output", "vec4"));
    // And the magic name really is that word, not one that happens to contain it.
    REQUIRE(std::string(PropertyNode::kOutputTypeName) == "output");
    REQUIRE(std::string(PropertyNode::kOutputTypeName) != "outputType");
    REQUIRE(PropertyNode::isPropertyNode);
}

TEST_CASE("PropertyNode's cache key joins three things with colons") {
    // Type, name and a digit for the varying flag.
    REQUIRE(PropertyNode::cacheKeyText("float", true, "threshold", false) == "float:threshold:0");
    REQUIRE(PropertyNode::cacheKeyText("float", true, "threshold", true) == "float:threshold:1");
    // Without a name the middle is empty rather than the word for absence.
    REQUIRE(PropertyNode::cacheKeyText("float", false, "", false) == "float::0");
    REQUIRE(PropertyNode::cacheKeyText("float", false, "", false) != "float:null:0");
    // The flag really distinguishes the two, so a varying and a plain property with the same name
    // do not share a key.
    REQUIRE(PropertyNode::cacheKeyText("float", true, "t", true) !=
            PropertyNode::cacheKeyText("float", true, "t", false));
    REQUIRE(std::string(PropertyNode::kTypeName) == "PropertyNode");
}

TEST_CASE("PropertyNode sets a varying's interpolation without asking") {
    // The varying branch assigns the interpolation flag true with no test of what it holds.
    REQUIRE(PropertyNode::isVarying(true));
    REQUIRE_FALSE(PropertyNode::isVarying(false));
    REQUIRE(PropertyNode::interpolationSetUnconditionally());
}

TEST_CASE("PropertyNode writes its placeholder only when nothing else writes") {
    // No placeholder, nothing to write.
    REQUIRE_FALSE(PropertyNode::writesPlaceholder(false, false));
    // A placeholder and no write elsewhere: it is written, which gives the property a starting
    // value.
    REQUIRE(PropertyNode::writesPlaceholder(true, false));
    // A placeholder but a write elsewhere: it is not, since the property is given its value
    // there.
    REQUIRE_FALSE(PropertyNode::writesPlaceholder(true, true));
    REQUIRE_FALSE(PropertyNode::writesPlaceholder(false, true));
    // So the two conditions have to hold together, and neither alone is enough.
    REQUIRE(PropertyNode::writesPlaceholder(true, false) !=
            PropertyNode::writesPlaceholder(true, true));
    REQUIRE_FALSE(PropertyNode::isCacheable());
}