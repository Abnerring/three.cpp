#include "loaders/GCodeLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GCodeLoader reads an absolute or a relative coordinate") {
    // An absolute program replaces the current position with the written one.
    REQUIRE(GCodeLoader::absoluteValue(false, 10.0, 5.0) == 5.0);
    REQUIRE(GCodeLoader::absoluteValue(false, 0.0, 7.0) == 7.0);
    // A relative one adds the written amount to where the head already is.
    REQUIRE(GCodeLoader::absoluteValue(true, 10.0, 5.0) == 15.0);
    REQUIRE(GCodeLoader::absoluteValue(true, 10.0, -5.0) == 5.0);
    // The current position is the first argument and is ignored when absolute.
    REQUIRE(GCodeLoader::absoluteValue(false, 100.0, 5.0) ==
            GCodeLoader::absoluteValue(false, 0.0, 5.0));
}

TEST_CASE("GCodeLoader chooses a list by whether it is extruding") {
    REQUIRE(GCodeLoader::segmentList(true) == GCodeLoader::SegmentList::Extruded);
    REQUIRE(GCodeLoader::segmentList(false) == GCodeLoader::SegmentList::Path);
    // The names match the two the materials carry.
    REQUIRE(GCodeLoader::segmentListName(GCodeLoader::SegmentList::Extruded) == "extruded");
    REQUIRE(GCodeLoader::segmentListName(GCodeLoader::SegmentList::Path) == "path");
}

TEST_CASE("GCodeLoader appends both ends of a segment") {
    GCodeLoader::Layer layer;
    const double from[3] = {1, 2, 3};
    const double to[3] = {4, 5, 6};
    // Extruding puts it in the first list, six numbers at a time.
    GCodeLoader::addSegment(layer, true, from, to);
    REQUIRE(layer.extruded.size() == 6);
    REQUIRE(layer.extruded[0] == 1);
    REQUIRE(layer.extruded[2] == 3);
    REQUIRE(layer.extruded[3] == 4);
    REQUIRE(layer.extruded[5] == 6);
    REQUIRE(layer.path.empty());
}

TEST_CASE("GCodeLoader keeps the two lists apart") {
    GCodeLoader::Layer layer;
    const double from[3] = {0, 0, 0};
    const double to[3] = {1, 1, 0};
    // A travel move goes in the path list, not the extruded one.
    GCodeLoader::addSegment(layer, false, from, to);
    REQUIRE(layer.path.size() == 6);
    REQUIRE(layer.extruded.empty());
    // Two segments of one kind make twelve numbers, not one merged run of points.
    GCodeLoader::addSegment(layer, false, to, from);
    REQUIRE(layer.path.size() == 12);
    REQUIRE(layer.path[6] == 1);
    REQUIRE(layer.path[9] == 0);
}

TEST_CASE("GCodeLoader starts a layer at the height it began at") {
    // The height is the one on the move that opened the layer, not a value the program states.
    GCodeLoader::Layer layer;
    layer.z = 0.3;
    REQUIRE(layer.z == 0.3);
    // A fresh layer has both lists empty.
    GCodeLoader::Layer fresh;
    REQUIRE(fresh.extruded.empty());
    REQUIRE(fresh.path.empty());
    REQUIRE(fresh.z == 0.0);
}