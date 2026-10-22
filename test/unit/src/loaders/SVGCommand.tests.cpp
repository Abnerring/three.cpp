#include "loaders/SVGLoader.h"

#include "catch.hpp"

TEST_CASE("SVGLoader names each command letter") {
    REQUIRE(SVGLoader::commandKind('M') == SVGLoader::CommandKind::Move);
    REQUIRE(SVGLoader::commandKind('L') == SVGLoader::CommandKind::Line);
    REQUIRE(SVGLoader::commandKind('H') == SVGLoader::CommandKind::Horizontal);
    REQUIRE(SVGLoader::commandKind('V') == SVGLoader::CommandKind::Vertical);
    REQUIRE(SVGLoader::commandKind('C') == SVGLoader::CommandKind::Cubic);
    REQUIRE(SVGLoader::commandKind('S') == SVGLoader::CommandKind::SmoothCubic);
    REQUIRE(SVGLoader::commandKind('Q') == SVGLoader::CommandKind::Quadratic);
    REQUIRE(SVGLoader::commandKind('T') == SVGLoader::CommandKind::SmoothQuadratic);
    REQUIRE(SVGLoader::commandKind('A') == SVGLoader::CommandKind::Arc);
    REQUIRE(SVGLoader::commandKind('Z') == SVGLoader::CommandKind::Close);
}

TEST_CASE("SVGLoader reads a lowercase letter as the same command") {
    // The letter's case says how it is measured, not which command it is.
    REQUIRE(SVGLoader::commandKind('m') == SVGLoader::CommandKind::Move);
    REQUIRE(SVGLoader::commandKind('z') == SVGLoader::CommandKind::Close);
    REQUIRE(SVGLoader::commandKind('a') == SVGLoader::CommandKind::Arc);
    REQUIRE(SVGLoader::commandKind('s') == SVGLoader::CommandKind::SmoothCubic);
}

TEST_CASE("SVGLoader tells the relative form") {
    // A lowercase letter is the relative form.
    REQUIRE(SVGLoader::isRelativeCommand('m'));
    REQUIRE(SVGLoader::isRelativeCommand('c'));
    REQUIRE(SVGLoader::isRelativeCommand('z'));
    // An uppercase one is not.
    REQUIRE_FALSE(SVGLoader::isRelativeCommand('M'));
    REQUIRE_FALSE(SVGLoader::isRelativeCommand('C'));
    REQUIRE_FALSE(SVGLoader::isRelativeCommand('Z'));
}

TEST_CASE("SVGLoader rejects a letter that names no command") {
    REQUIRE(SVGLoader::commandKind('X') == SVGLoader::CommandKind::Unknown);
    REQUIRE(SVGLoader::commandKind(' ') == SVGLoader::CommandKind::Unknown);
    REQUIRE(SVGLoader::commandKind('0') == SVGLoader::CommandKind::Unknown);
    // A letter that names no command is not a command letter.
    REQUIRE_FALSE(SVGLoader::isCommandLetter('X'));
    REQUIRE_FALSE(SVGLoader::isCommandLetter('x'));
    // And a lowercase non command is not a relative form either.
    REQUIRE_FALSE(SVGLoader::isRelativeCommand('x'));
    REQUIRE_FALSE(SVGLoader::isRelativeCommand(' '));
}

TEST_CASE("SVGLoader recognises every command letter") {
    // All ten count, in both cases.
    for (const char letter : {'M', 'L', 'H', 'V', 'C', 'S', 'Q', 'T', 'A', 'Z'}) {
        REQUIRE(SVGLoader::isCommandLetter(letter));
        REQUIRE_FALSE(SVGLoader::isRelativeCommand(letter));
    }
    for (const char letter : {'m', 'l', 'h', 'v', 'c', 's', 'q', 't', 'a', 'z'}) {
        REQUIRE(SVGLoader::isCommandLetter(letter));
        REQUIRE(SVGLoader::isRelativeCommand(letter));
    }
}
