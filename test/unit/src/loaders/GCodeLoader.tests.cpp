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
TEST_CASE("GCodeLoader chooses the extrusion reading from three flags") {
    // Without an override the global setting decides.
    REQUIRE_FALSE(GCodeLoader::extrusionIsRelative(false, true, false));
    REQUIRE(GCodeLoader::extrusionIsRelative(false, false, true));
    // With one the extruder's own setting wins.
    REQUIRE(GCodeLoader::extrusionIsRelative(true, true, false));
    REQUIRE_FALSE(GCodeLoader::extrusionIsRelative(true, false, true));
    // The value follows the same choice.
    REQUIRE(GCodeLoader::absoluteExtrusion(true, true, false, 10.0, 5.0) == 15.0);
    REQUIRE(GCodeLoader::absoluteExtrusion(true, false, true, 10.0, 5.0) == 5.0);
}

TEST_CASE("GCodeLoader strips a comment") {
    // Everything from the semicolon onwards goes, the space before it staying.
    REQUIRE(GCodeLoader::stripComment("G1 X5 ; a note") == "G1 X5 ");
    REQUIRE(GCodeLoader::stripComment("; only a comment") == "");
    // A line without one is unchanged.
    REQUIRE(GCodeLoader::stripComment("G1 X5") == "G1 X5");
    REQUIRE(GCodeLoader::stripComment("") == "");
}

TEST_CASE("GCodeLoader reads one argument") {
    // A letter and a number straight after it.
    const auto x = GCodeLoader::parseArgument("X12.5");
    REQUIRE(x.present);
    REQUIRE(x.letter == 'x');
    REQUIRE(x.value == 12.5);
    // The letter is lower cased whatever case it was written in.
    REQUIRE(GCodeLoader::parseArgument("e3").letter == 'e');
    REQUIRE(GCodeLoader::parseArgument("E3").letter == 'e');
    // An empty token has no letter at all, which is what a run of spaces leaves behind.
    REQUIRE_FALSE(GCodeLoader::parseArgument("").present);
    // A whole number parses the same way a fractional one does.
    REQUIRE(GCodeLoader::parseArgument("Y7").value == 7.0);
}

TEST_CASE("GCodeLoader decides extrusion by the change in amount") {
    // More than before is extruding.
    REQUIRE(GCodeLoader::isExtruding(1.0, 2.0));
    // Less and the same are not.
    REQUIRE_FALSE(GCodeLoader::isExtruding(2.0, 1.0));
    REQUIRE_FALSE(GCodeLoader::isExtruding(1.0, 1.0));
    // A retraction, which is a decrease, is not extruding either.
    REQUIRE_FALSE(GCodeLoader::isExtruding(1.0, 0.5));
}

TEST_CASE("GCodeLoader starts a layer only when extruding at a new height") {
    // The first extruding step opens the first layer whatever its height.
    REQUIRE(GCodeLoader::beginsLayer(false, 0.0, 0.3, true));
    // At the same height it does not.
    REQUIRE_FALSE(GCodeLoader::beginsLayer(true, 0.3, 0.3, true));
    // At a new height it does.
    REQUIRE(GCodeLoader::beginsLayer(true, 0.3, 0.6, true));
    // A travel move never does, however far it moves.
    REQUIRE_FALSE(GCodeLoader::beginsLayer(true, 0.3, 0.6, false));
    REQUIRE_FALSE(GCodeLoader::beginsLayer(false, 0.0, 5.0, false));
}
