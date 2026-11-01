#include "loaders/SVGLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SVGLoader reads a gradient reference") {
    // The plain form.
    REQUIRE(SVGLoader::gradientId("url(#a)") == "a");
    // Whitespace on both sides of the whole value is allowed.
    REQUIRE(SVGLoader::gradientId("  url(#a)  ") == "a");
    // Whitespace inside the parentheses is allowed too.
    REQUIRE(SVGLoader::gradientId("url(  #a  )") == "a");
}

TEST_CASE("SVGLoader accepts a quoted identifier") {
    // Either kind of quote, on either side of the hash.
    REQUIRE(SVGLoader::gradientId("url('#a')") == "a");
    REQUIRE(SVGLoader::gradientId("url(\"#a\")") == "a");
    // Whitespace may sit between the quote and the identifier.
    REQUIRE(SVGLoader::gradientId("url( '#a' )") == "a");
}

TEST_CASE("SVGLoader keeps the punctuation inside an identifier") {
    // Dashes and underscores are part of the name.
    REQUIRE(SVGLoader::gradientId("url(#a-b_c)") == "a-b_c");
    // Digits are too.
    REQUIRE(SVGLoader::gradientId("url(#grad2)") == "grad2");
}

TEST_CASE("SVGLoader rejects a value that is not a reference") {
    REQUIRE(SVGLoader::gradientId("").empty());
    REQUIRE(SVGLoader::gradientId("none").empty());
    REQUIRE(SVGLoader::gradientId("#a").empty());
    REQUIRE(SVGLoader::gradientId("url(a)").empty());
}

TEST_CASE("SVGLoader rejects an empty or unterminated identifier") {
    // A hash with nothing after it names nothing.
    REQUIRE(SVGLoader::gradientId("url(#)").empty());
    // A reference with no closing bracket is not one.
    REQUIRE(SVGLoader::gradientId("url(#a").empty());
    // Nor is one with a stray word after it.
    REQUIRE(SVGLoader::gradientId("url(#a) red").empty());
}

TEST_CASE("SVGLoader rejects two references in one value") {
    // The value has to be a single reference, so a second one disqualifies it.
    REQUIRE(SVGLoader::gradientId("url(#a) url(#b)").empty());
    // Even a trailing word does.
    REQUIRE(SVGLoader::gradientId("url(#a)X").empty());
}

TEST_CASE("SVGLoader names the colour space") {
    // The loader reads fills in this space, which is the display space.
    REQUIRE(std::string(SVGLoader::kColorSpace) == "srgb");
}