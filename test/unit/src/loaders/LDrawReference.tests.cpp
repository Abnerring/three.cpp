#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader reads a reference line") {
    // The directive itself was consumed by the caller, so the line starts at the colour.
    // Sixteen is the inherited colour, then a position, an identity rotation and a part.
    LDrawLoader::LineParser parser("16 10 20 30 1 0 0 0 1 0 0 0 1 3001.dat", 5);
    const auto reference = LDrawLoader::parseReferenceLine(parser);

    REQUIRE(reference.colorCode == "16");
    REQUIRE(reference.position[0] == 10.0);
    REQUIRE(reference.position[1] == 20.0);
    REQUIRE(reference.position[2] == 30.0);
    // The rotation comes row by row.
    REQUIRE(reference.rotation[0] == 1.0);
    REQUIRE(reference.rotation[1] == 0.0);
    REQUIRE(reference.rotation[4] == 1.0);
    REQUIRE(reference.rotation[8] == 1.0);
    REQUIRE(reference.fileName == "3001.dat");
    REQUIRE(parser.isAtTheEnd());
}

TEST_CASE("LDrawLoader keeps a colour name as written") {
    // A colour may be named rather than numbered.
    LDrawLoader::LineParser parser("main 0 0 0 1 0 0 0 1 0 0 0 1 part.dat", 6);
    const auto reference = LDrawLoader::parseReferenceLine(parser);
    REQUIRE(reference.colorCode == "main");
    REQUIRE(reference.position[0] == 0.0);
    REQUIRE(reference.fileName == "part.dat");
}

TEST_CASE("LDrawLoader reads a scaled rotation") {
    // A matrix that scales by two on every axis.
    LDrawLoader::LineParser parser("4 1 2 3 2 0 0 0 2 0 0 0 2 big.dat", 7);
    const auto reference = LDrawLoader::parseReferenceLine(parser);
    REQUIRE(reference.colorCode == "4");
    REQUIRE(reference.position[0] == 1.0);
    REQUIRE(reference.position[2] == 3.0);
    REQUIRE(reference.rotation[0] == 2.0);
    REQUIRE(reference.rotation[4] == 2.0);
    REQUIRE(reference.rotation[8] == 2.0);
    REQUIRE(reference.fileName == "big.dat");
}

TEST_CASE("LDrawLoader fills a missing tail with zeros") {
    // A truncated line still parses; the missing tokens read as zero and the name is empty.
    LDrawLoader::LineParser parser("16 1 2", 8);
    const auto reference = LDrawLoader::parseReferenceLine(parser);
    REQUIRE(reference.colorCode == "16");
    REQUIRE(reference.position[0] == 1.0);
    REQUIRE(reference.position[2] == 0.0);
    REQUIRE(reference.rotation[0] == 0.0);
    REQUIRE(reference.fileName.empty());
}
