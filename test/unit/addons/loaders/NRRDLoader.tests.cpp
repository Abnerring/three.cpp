#include "loaders/NRRDLoader.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kHeader =
    "NRRD0005\n"
    "# a comment\n"
    "type: float\n"
    "dimension: 3\n"
    "sizes: 2 3 4\n"
    "spacings: 10 20 30\n"
    "encoding: raw\n"
    "\n";
}

TEST_CASE("NRRDLoader reads the key and value lines") {
    const auto header = NRRDLoader::parseHeader(kHeader);
    REQUIRE(header.isNrrd);
    REQUIRE(header.fields.at("type") == "float");
    REQUIRE(header.dimension == 3);
    REQUIRE(header.sizes.size() == 3);
    REQUIRE(header.sizes[0] == 2.0);
    REQUIRE(header.sizes[2] == 4.0);
    REQUIRE(header.encoding == "raw");
    // The typed keys are also kept as free form fields.
    REQUIRE(header.fields.at("encoding") == "raw");
}

TEST_CASE("NRRDLoader defaults the vectors to the identity") {
    auto header = NRRDLoader::parseHeader("NRRD0005\ntype: float\nencoding: raw\n");
    NRRDLoader::resolveVectors(header);
    REQUIRE(header.vectors.size() == 3);
    REQUIRE(header.vectors[0][0] == 1.0);
    REQUIRE(header.vectors[1][1] == 1.0);
    REQUIRE(header.vectors[2][2] == 1.0);
    REQUIRE(header.vectors[0][1] == 0.0);
}

TEST_CASE("NRRDLoader scales each row by its spacing") {
    auto header = NRRDLoader::parseHeader(kHeader);
    NRRDLoader::resolveVectors(header);
    REQUIRE(header.vectors.size() == 3);
    // The spacings are ten, twenty and thirty along the three axes.
    REQUIRE(header.vectors[0][0] == 10.0);
    REQUIRE(header.vectors[1][1] == 20.0);
    REQUIRE(header.vectors[2][2] == 30.0);
    REQUIRE(header.vectors[1][0] == 0.0);
}

TEST_CASE("NRRDLoader rejects a bzip encoding") {
    REQUIRE_THROWS_WITH(NRRDLoader::parseHeader("NRRD0005\nencoding: bz2\n"),
                        "THREE.NRRDLoader: Bzip is not supported");
    REQUIRE_THROWS_WITH(NRRDLoader::parseHeader("NRRD0005\nencoding: bzip2\n"),
                        "THREE.NRRDLoader: Bzip is not supported");
}

TEST_CASE("NRRDLoader rejects a file without the magic") {
    REQUIRE_THROWS_WITH(NRRDLoader::parseHeader("type: float\nencoding: raw\n"),
                        "THREE.NRRDLoader: Not an NRRD file");
}
