#include "loaders/PCDLoader.h"

#include "catch.hpp"

#include <string>

namespace {
PCDLoader::FieldLayout xyzRgb() {
    PCDLoader::FieldLayout layout;
    layout.offset["x"] = 0;
    layout.offset["y"] = 1;
    layout.offset["z"] = 2;
    layout.offset["rgb"] = 3;
    return layout;
}
}  // namespace

TEST_CASE("PCDLoader unpacks a packed rgb value") {
    int rgb[3] = {0, 0, 0};
    // Red in the highest byte, then green, then blue.
    PCDLoader::unpackRGB(0xff8040u, rgb);
    REQUIRE(rgb[0] == 255);
    REQUIRE(rgb[1] == 128);
    REQUIRE(rgb[2] == 64);
}

TEST_CASE("PCDLoader reads ascii positions and colours") {
    // Two rows; the second colour is pure red.
    const std::string text = "1 2 3 16777215\n4 5 6 16711680\n";
    const auto points = PCDLoader::parseAsciiPoints(text, xyzRgb());
    REQUIRE(points.positions.size() == 6);
    REQUIRE(points.positions[0] == 1.0);
    REQUIRE(points.positions[5] == 6.0);
    REQUIRE(points.colors.size() == 6);
    // White becomes one in every channel after the conversion.
    REQUIRE(points.colors[0] == Approx(1.0).margin(1e-6));
    REQUIRE(points.colors[1] == Approx(1.0).margin(1e-6));
    // Pure red keeps red at one and drops the other two.
    REQUIRE(points.colors[3] == Approx(1.0).margin(1e-6));
    REQUIRE(points.colors[4] == Approx(0.0).margin(1e-6));
    REQUIRE(points.colors[5] == Approx(0.0).margin(1e-6));
}

TEST_CASE("PCDLoader skips empty lines and handles missing fields") {
    PCDLoader::FieldLayout layout;
    layout.offset["x"] = 0;
    layout.offset["y"] = 1;
    layout.offset["z"] = 2;
    // No rgb, no normals, so only positions come out.
    const auto points = PCDLoader::parseAsciiPoints("\n1 2 3\n\n4 5 6\n", layout);
    REQUIRE(points.positions.size() == 6);
    REQUIRE(points.colors.empty());
    REQUIRE(points.normals.empty());
}

TEST_CASE("PCDLoader reads normals, intensity and label") {
    PCDLoader::FieldLayout layout;
    layout.offset["x"] = 0;
    layout.offset["y"] = 1;
    layout.offset["z"] = 2;
    layout.offset["normal_x"] = 3;
    layout.offset["normal_y"] = 4;
    layout.offset["normal_z"] = 5;
    layout.offset["intensity"] = 6;
    layout.offset["label"] = 7;
    const auto points = PCDLoader::parseAsciiPoints("1 2 3 0 1 0 42 7\n", layout);
    REQUIRE(points.normals.size() == 3);
    REQUIRE(points.normals[1] == 1.0);
    REQUIRE(points.intensity.size() == 1);
    REQUIRE(points.intensity[0] == 42.0);
    REQUIRE(points.label.size() == 1);
    REQUIRE(points.label[0] == 7.0);
}

TEST_CASE("PCDLoader needs all three position components") {
    PCDLoader::FieldLayout layout;
    layout.offset["x"] = 0;
    layout.offset["y"] = 1;
    // No z, so no position is recorded even though the row has values.
    const auto points = PCDLoader::parseAsciiPoints("1 2 3\n", layout);
    REQUIRE(points.positions.empty());
}

TEST_CASE("PCDLoader reads nothing from empty text") {
    REQUIRE(PCDLoader::parseAsciiPoints("", xyzRgb()).positions.empty());
}
