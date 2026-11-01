#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader cuts a line into tokens") {
    // A short line so the tokens are easy to follow.
    LDrawLoader::LineParser parser("1 4 0 part.dat", 7);
    // The directive comes first.
    REQUIRE(parser.getToken() == "1");
    // Then the colour code.
    REQUIRE(parser.getToken() == "4");
    // Then the matrix and the file name.
    REQUIRE(parser.getToken() == "0");
    REQUIRE(parser.getToken() == "part.dat");
    // The name was the last token.
    REQUIRE(parser.isAtTheEnd());
}

TEST_CASE("LDrawLoader skips runs of whitespace") {
    // Tabs and several spaces count as one separator.
    LDrawLoader::LineParser parser("0  FILE   name.dat", 1);
    REQUIRE(parser.getToken() == "0");
    REQUIRE(parser.getToken() == "FILE");
    REQUIRE(parser.getToken() == "name.dat");
}

TEST_CASE("LDrawLoader reads a vector of three numbers") {
    LDrawLoader::LineParser parser("0 0 0 1 0 0", 2);
    // Three zeros.
    const auto origin = parser.getVector();
    REQUIRE(origin[0] == 0.0);
    REQUIRE(origin[1] == 0.0);
    REQUIRE(origin[2] == 0.0);
    // One, zero, zero.
    const auto axis = parser.getVector();
    REQUIRE(axis[0] == 1.0);
    REQUIRE(axis[1] == 0.0);
    REQUIRE(axis[2] == 0.0);
}

TEST_CASE("LDrawLoader gives an empty token at the end") {
    LDrawLoader::LineParser parser("one", 3);
    REQUIRE(parser.getToken() == "one");
    REQUIRE(parser.isAtTheEnd());
    // Nothing remains, so the token is empty rather than an error.
    REQUIRE(parser.getToken().empty());
    REQUIRE(parser.getToken().empty());
}

TEST_CASE("LDrawLoader takes the rest of the line as one piece") {
    LDrawLoader::LineParser parser("0  the rest of it", 4);
    REQUIRE(parser.getToken() == "0");
    // Everything after the first token, whitespace included.
    REQUIRE(parser.getRemainingString() == "the rest of it");
    // Once at the end there is nothing left.
    parser.setToEnd();
    REQUIRE(parser.getRemainingString().empty());
    REQUIRE(parser.isAtTheEnd());
}

TEST_CASE("LDrawLoader names the line only when it has a number") {
    const LDrawLoader::LineParser numbered("x", 42);
    REQUIRE(numbered.getLineNumberString() == " at line 42");
    // A negative number means the line has no number to report.
    const LDrawLoader::LineParser unnumbered("x", -1);
    REQUIRE(unnumbered.getLineNumberString().empty());
    // Zero is a real number.
    const LDrawLoader::LineParser zero("x", 0);
    REQUIRE(zero.getLineNumberString() == " at line 0");
}

TEST_CASE("LDrawLoader starts at the beginning of the line") {
    const LDrawLoader::LineParser parser("abc", 1);
    // The cursor begins before the first character, not on it.
    REQUIRE(parser.position() == 0);
    REQUIRE_FALSE(parser.isAtTheEnd());
}

TEST_CASE("LDrawLoader handles an empty line") {
    LDrawLoader::LineParser parser("", 1);
    REQUIRE(parser.isAtTheEnd());
    REQUIRE(parser.getToken().empty());
    REQUIRE(parser.getRemainingString().empty());
    parser.seekNonSpace();
    REQUIRE(parser.isAtTheEnd());
}
