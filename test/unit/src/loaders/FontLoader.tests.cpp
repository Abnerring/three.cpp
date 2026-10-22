#include "loaders/FontLoader.h"

#include "catch.hpp"

#include <string>

namespace {
Font::Data makeFont() {
    Font::Data data;
    data.familyName = "Test";
    data.resolution = 100;
    data.boundingBoxYMin = -20;
    data.boundingBoxYMax = 80;
    data.underlineThickness = 5;
    data.ascender = 90;
    data.glyphs["A"] = Font::Glyph{50, ""};
    data.glyphs["B"] = Font::Glyph{30, ""};
    data.glyphs["?"] = Font::Glyph{10, ""};
    return data;
}
}  // namespace

TEST_CASE("Font reports its type and keeps the data") {
    Font font(makeFont());
    REQUIRE(Font::isFont);
    REQUIRE(font.type == "Font");
    REQUIRE(font.data.familyName == "Test");
}

TEST_CASE("Font computes the line height from the bounding box") {
    Font font(makeFont());
    // (80 - (-20) + 5) * (100 / 100) = 105
    REQUIRE(font.lineHeight(100) == 105.0);
    // At half size the same box gives half the height.
    REQUIRE(font.lineHeight(50) == 52.5);
}

TEST_CASE("Font places characters left to right by their advance") {
    Font font(makeFont());
    const auto placed = font.layoutText("AB", 100, "ltr");
    REQUIRE(placed.size() == 2);
    REQUIRE(placed[0].character == "A");
    REQUIRE(placed[0].x == 0);
    REQUIRE(placed[0].y == 0);
    // A advances by ha * scale, so B starts at 50.
    REQUIRE(placed[1].x == 50);
    REQUIRE(placed[1].y == 0);
}

TEST_CASE("Font advances the offset by a line on a newline") {
    Font font(makeFont());
    const auto placed = font.layoutText("A\nB", 100, "ltr");
    REQUIRE(placed.size() == 2);
    REQUIRE(placed[0].x == 0);
    // B starts at the left edge again and a full line lower.
    REQUIRE(placed[1].x == 0);
    REQUIRE(placed[1].y == -105);
}

TEST_CASE("Font reverses the order for right to left and top to bottom") {
    Font font(makeFont());
    const auto rtl = font.layoutText("AB", 100, "rtl");
    REQUIRE(rtl.size() == 2);
    // The characters are reversed, so B is placed first.
    REQUIRE(rtl[0].character == "B");
    REQUIRE(rtl[0].x == 0);
    REQUIRE(rtl[1].character == "A");

    const auto tb = font.layoutText("AB", 100, "tb");
    REQUIRE(tb.size() == 2);
    // Top down advances vertically by the ascender instead of horizontally.
    REQUIRE(tb[0].y == 0);
    REQUIRE(tb[1].x == 0);
    REQUIRE(tb[1].y == 90);
}

TEST_CASE("Font falls back to the question mark and splits code points") {
    Font font(makeFont());
    // An unknown character is drawn with the fallback glyph but keeps its own name.
    const auto placed = font.layoutText("Z", 100, "ltr");
    REQUIRE(placed.size() == 1);
    REQUIRE(placed[0].character == "Z");
    // The fallback advance is 10, so the next character would start there.
    const auto two = font.layoutText("ZZ", 100, "ltr");
    REQUIRE(two[1].x == 10);

    // A two byte character counts as one code point.
    const auto points = Font::codePoints("A\xC3\xA9");
    REQUIRE(points.size() == 2);
    REQUIRE(points[1].size() == 2);
}
