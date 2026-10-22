#include "loaders/GCodeLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GCodeLoader returns nothing for an empty program") {
    GCodeLoader loader;
    const auto result = loader.parse("");
    REQUIRE(result.layers.empty());
    REQUIRE(result.vertex.empty());
    REQUIRE(result.pathVertex.empty());
    REQUIRE(result.finalState.x == 0);
}

TEST_CASE("GCodeLoader records the first extruding move as one segment") {
    GCodeLoader loader;
    // The move from the origin to ten on X is the first extrusion, so it opens a layer and
    // lands in the extruding stream.
    const auto result = loader.parse("G90\nG1 X10 E1\n");
    REQUIRE(result.layers.size() == 1);
    REQUIRE(result.layers[0].z == 0);
    REQUIRE(result.layers[0].vertex.size() == 6);
    REQUIRE(result.layers[0].vertex[0] == 0);
    REQUIRE(result.layers[0].vertex[3] == 10);
    REQUIRE(result.layers[0].pathVertex.empty());
    REQUIRE(result.vertex.size() == 6);
}

TEST_CASE("GCodeLoader starts a new layer when extrusion moves to a new Z") {
    GCodeLoader loader;
    const auto result = loader.parse("G90\nG1 X10 E1\nG1 Z1 E2\n");
    // The second extrusion happens at a different Z, which upstream treats as a new layer.
    REQUIRE(result.layers.size() == 2);
    REQUIRE(result.layers[0].z == 0);
    REQUIRE(result.layers[1].z == 1);
    REQUIRE(result.layers[1].vertex.size() == 6);
    // The concatenated stream holds both segments.
    REQUIRE(result.vertex.size() == 12);
}

TEST_CASE("GCodeLoader honours relative positioning and the extrusion override") {
    GCodeLoader loader;
    const auto relativeMove = loader.parse("G91\nG1 X5 E1\nG1 X5\n");
    // With G91 every coordinate is added to the previous one.
    REQUIRE(relativeMove.finalState.x == 10);
    REQUIRE_FALSE(relativeMove.finalState.relative == false);

    GCodeLoader second;
    const auto overridden = second.parse("G91\nM82\nG1 X5 E1\n");
    // M82 forces the extrusion axis back to absolute even though the axes are relative.
    REQUIRE(overridden.finalState.extrusionOverride);
    REQUIRE_FALSE(overridden.finalState.extrusionRelative);
    REQUIRE(overridden.finalState.e == 1);
}

TEST_CASE("GCodeLoader applies G92 without emitting a segment") {
    GCodeLoader loader;
    const auto result = loader.parse("G92 X5 Y6 Z7 E8\n");
    REQUIRE(result.layers.empty());
    REQUIRE(result.finalState.x == 5);
    REQUIRE(result.finalState.y == 6);
    REQUIRE(result.finalState.z == 7);
    REQUIRE(result.finalState.e == 8);
}

TEST_CASE("GCodeLoader strips comments before parsing") {
    GCodeLoader loader;
    const auto result = loader.parse("G90 ; absolute mode\nG1 X3 E1 ; first move\n");
    REQUIRE(result.finalState.x == 3);
    REQUIRE(result.vertex.size() == 6);
}
