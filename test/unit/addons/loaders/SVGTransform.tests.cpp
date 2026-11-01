#include "loaders/SVGLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SVGLoader reads one transform") {
    const auto ops = SVGLoader::parseTransformList("translate(1,2)");
    REQUIRE(ops.size() == 1);
    REQUIRE(ops[0].name == "translate");
    REQUIRE(ops[0].args.size() == 2);
    REQUIRE(ops[0].args[0] == 1.0);
    REQUIRE(ops[0].args[1] == 2.0);
}

TEST_CASE("SVGLoader reads the transforms from the right") {
    // Two transforms, written left to right, come back right to left.
    const auto ops = SVGLoader::parseTransformList("translate(1) rotate(45)");
    REQUIRE(ops.size() == 2);
    REQUIRE(ops[0].name == "rotate");
    REQUIRE(ops[0].args[0] == 45.0);
    REQUIRE(ops[1].name == "translate");
    REQUIRE(ops[1].args[0] == 1.0);
}

TEST_CASE("SVGLoader keeps three transforms in reverse") {
    const auto ops = SVGLoader::parseTransformList("translate(1) scale(2) rotate(3)");
    REQUIRE(ops.size() == 3);
    REQUIRE(ops[0].name == "rotate");
    REQUIRE(ops[1].name == "scale");
    REQUIRE(ops[2].name == "translate");
}

TEST_CASE("SVGLoader reads the whitespace around a transform") {
    // The pieces are trimmed before they are read.
    const auto ops = SVGLoader::parseTransformList("  translate( 1 , 2 )  ");
    REQUIRE(ops.size() == 1);
    REQUIRE(ops[0].name == "translate");
    REQUIRE(ops[0].args.size() == 2);
    REQUIRE(ops[0].args[1] == 2.0);
}

TEST_CASE("SVGLoader skips what is not a transform") {
    // Nothing at all.
    REQUIRE(SVGLoader::parseTransformList("").empty());
    REQUIRE(SVGLoader::parseTransformList("   ").empty());
    // A word with no bracket.
    REQUIRE(SVGLoader::parseTransformList("translate").empty());
    // A bracket with nothing before it has no name.
    REQUIRE(SVGLoader::parseTransformList("(1)").empty());
    // An empty bracket pair after a real transform contributes nothing of its own.
    const auto mixed = SVGLoader::parseTransformList("translate(1))");
    REQUIRE(mixed.size() == 1);
    REQUIRE(mixed[0].name == "translate");
}

TEST_CASE("SVGLoader reads a transform with no arguments") {
    // The name is still there, with nothing to apply.
    const auto ops = SVGLoader::parseTransformList("translate()");
    REQUIRE(ops.size() == 1);
    REQUIRE(ops[0].name == "translate");
    REQUIRE(ops[0].args.empty());
}