#include "loaders/NRRDLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NRRDLoader prefers explicit directions over the spacing") {
    auto header = NRRDLoader::parseHeader(
        "NRRD0005\nspacings: 10 10 10\nspace directions: (2,0,0) (0,3,0) (0,0,4)\n");
    NRRDLoader::resolveVectors(header);
    // The declared directions are used as they are; the spacing does not scale them again.
    REQUIRE(header.vectors.size() == 3);
    REQUIRE(header.vectors[0][0] == 2.0);
    REQUIRE(header.vectors[1][1] == 3.0);
    REQUIRE(header.vectors[2][2] == 4.0);
}

TEST_CASE("NRRDLoader keeps an identity row for none") {
    auto header = NRRDLoader::parseHeader("NRRD0005\nspace directions: none (0,1,0) (0,0,1)\n");
    NRRDLoader::resolveVectors(header);
    REQUIRE(header.vectors.size() == 3);
    // The axis without a direction points along itself.
    REQUIRE(header.vectors[0][0] == 1.0);
    REQUIRE(header.vectors[0][1] == 0.0);
    REQUIRE(header.vectors[1][1] == 1.0);
    REQUIRE(header.vectors[2][2] == 1.0);
}

TEST_CASE("NRRDLoader parses fractional directions") {
    auto header = NRRDLoader::parseHeader(
        "NRRD0005\nspace directions: (0.5,0,0) (0,0.5,0) (0,0,0.5)\n");
    NRRDLoader::resolveVectors(header);
    REQUIRE(header.vectors[0][0] == 0.5);
    REQUIRE(header.vectors[2][2] == 0.5);
}

TEST_CASE("NRRDLoader still applies spacing when no directions are given") {
    auto header = NRRDLoader::parseHeader("NRRD0005\nspacings: 5 5 5\n");
    NRRDLoader::resolveVectors(header);
    REQUIRE(header.vectors[0][0] == 5.0);
    REQUIRE(header.vectors[2][2] == 5.0);
}
