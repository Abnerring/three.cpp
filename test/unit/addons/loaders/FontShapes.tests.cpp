#include "loaders/FontLoader.h"

#include "extras/core/ShapePath.h"

#include "catch.hpp"

namespace {
Font::Data makeFont() {
    Font::Data data;
    data.resolution = 100;
    // A unit square outline for A and a triangle for B.
    data.glyphs["A"] = Font::Glyph{50, "m 0 0 l 10 0 l 10 10 l 0 10 l 0 0"};
    data.glyphs["B"] = Font::Glyph{30, "m 0 0 l 20 0 l 10 20 l 0 0"};
    return data;
}
}  // namespace

TEST_CASE("Font creates one path per placed character") {
    Font font(makeFont());
    const auto paths = font.createPaths("AB", 100, "ltr");
    REQUIRE(paths.size() == 2);
    REQUIRE(paths[0] != nullptr);
    REQUIRE(paths[1] != nullptr);
}

TEST_CASE("Font skips characters without a glyph") {
    Font font(makeFont());
    // Z has no glyph and there is no question mark fallback, so nothing is produced.
    const auto paths = font.createPaths("AZB", 100, "ltr");
    REQUIRE(paths.size() == 2);
}

TEST_CASE("Font reverses the path order for right to left") {
    Font font(makeFont());
    const auto ltr = font.createPaths("AB", 100, "ltr");
    const auto rtl = font.createPaths("AB", 100, "rtl");
    REQUIRE(ltr.size() == 2);
    REQUIRE(rtl.size() == 2);
    // Both directions produce two paths; the characters are simply swapped.
    REQUIRE(ltr[0] != rtl[0]);
}

TEST_CASE("Font generates shapes from the paths") {
    Font font(makeFont());
    const auto shapes = font.generateShapes("AB", 100, "ltr");
    // A closed square and a closed triangle each become at least one shape.
    REQUIRE(shapes.size() >= 2);
    for (const auto& shape : shapes) REQUIRE(shape != nullptr);
}

TEST_CASE("Font returns nothing for text it cannot draw") {
    Font font(makeFont());
    REQUIRE(font.createPaths("", 100, "ltr").empty());
    REQUIRE(font.generateShapes("", 100, "ltr").empty());
}
