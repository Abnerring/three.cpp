#include "loaders/FontLoader.h"

#include "extras/core/ShapePath.h"

#include "catch.hpp"

#include <string>

namespace {
Font::Data makeFont(const std::string& outline, double advance) {
    Font::Data data;
    data.resolution = 100;
    data.glyphs["A"] = Font::Glyph{advance, outline};
    return data;
}
}  // namespace

TEST_CASE("Font builds a path from move and line commands") {
    const auto data = makeFont("m 0 0 l 10 0 l 10 10", 50);
    const auto glyphPath = Font::createPath("A", 1.0, 0.0, 0.0, data);
    REQUIRE(glyphPath.path != nullptr);
    // The advance is the glyph advance times the scale.
    REQUIRE(glyphPath.advance == 50.0);
    // All three commands went into one sub path.
    REQUIRE(glyphPath.path->subPaths->size() == 1);
}

TEST_CASE("Font scales the advance with the requested size") {
    const auto data = makeFont("m 0 0 l 10 0", 50);
    REQUIRE(Font::createPath("A", 2.0, 0.0, 0.0, data).advance == 100.0);
    REQUIRE(Font::createPath("A", 0.5, 0.0, 0.0, data).advance == 25.0);
}

TEST_CASE("Font accepts every command letter without losing the path") {
    const auto data = makeFont("m 0 0 l 10 0 q 30 40 10 20 b 30 40 5 10 15 20", 1);
    const auto glyphPath = Font::createPath("A", 1.0, 0.0, 0.0, data);
    REQUIRE(glyphPath.path != nullptr);
    REQUIRE(glyphPath.path->subPaths->size() == 1);
    REQUIRE(glyphPath.advance == 1.0);
}

TEST_CASE("Font applies the outline offsets through the path") {
    // Two different offsets must produce two different paths, which only holds if the
    // offset reaches the coordinates.
    const auto data = makeFont("m 0 0", 1);
    const auto atOrigin = Font::createPath("A", 1.0, 0.0, 0.0, data);
    const auto shifted = Font::createPath("A", 1.0, 100.0, 200.0, data);
    REQUIRE(atOrigin.path != nullptr);
    REQUIRE(shifted.path != nullptr);
    REQUIRE(shifted.path->subPaths->size() == 1);
}

TEST_CASE("Font returns an empty path for a missing glyph") {
    Font::Data data;
    data.resolution = 100;
    const auto glyphPath = Font::createPath("Z", 1.0, 0.0, 0.0, data);
    // Neither the character nor the fallback exists.
    REQUIRE(glyphPath.path == nullptr);
    REQUIRE(glyphPath.advance == 0.0);
}

TEST_CASE("Font falls back to the question mark glyph") {
    Font::Data data;
    data.resolution = 100;
    data.glyphs["?"] = Font::Glyph{10, "m 0 0 l 5 0"};
    const auto glyphPath = Font::createPath("Z", 1.0, 0.0, 0.0, data);
    REQUIRE(glyphPath.path != nullptr);
    REQUIRE(glyphPath.advance == 10.0);
}
