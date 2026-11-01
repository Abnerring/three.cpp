#include "loaders/PDBLoader.h"

#include "catch.hpp"

#include <string>

namespace {
// Columns matter more than the words: element comes from 76 to 78, the coordinates from 30
// onwards, the serial from 6 to 11 and the bonds from 11 in five character fields.
const char* kPdb =
    "ATOM      1  N   ALA A   1      11.104  13.207  10.000  1.00  0.00           N\n"
    "ATOM      2  CA  ALA A   1      12.560  13.207  10.000  1.00  0.00           C\n"
    "ATOM      3  C   ALA A   1      13.000  14.000  10.000  1.00  0.00           C\n"
    "CONECT    1    2\n"
    "CONECT    2    3\n"
    "CONECT    1    2\n";
}

TEST_CASE("PDBLoader reads coordinates, serials and elements") {
    PDBLoader loader;
    const auto result = loader.parse(kPdb);
    REQUIRE(result.atoms.size() == 3);

    REQUIRE(result.atoms[0].x == Approx(11.104).epsilon(1e-9));
    REQUIRE(result.atoms[0].y == Approx(13.207).epsilon(1e-9));
    REQUIRE(result.atoms[0].z == Approx(10.0).epsilon(1e-9));
    REQUIRE(result.atoms[0].index == 1);
    REQUIRE(result.atoms[0].symbol == "n");
    REQUIRE(result.atoms[0].element == "N");

    REQUIRE(result.atoms[1].index == 2);
    REQUIRE(result.atoms[1].symbol == "c");
    REQUIRE(result.atoms[1].element == "C");
}

TEST_CASE("PDBLoader looks colours up in the CPK table") {
    PDBLoader loader;
    const auto result = loader.parse(kPdb);
    REQUIRE(result.atoms[0].cpk.size() == 3);
    // Nitrogen is 48, 80, 248 and carbon is 144, 144, 144 upstream.
    REQUIRE(result.atoms[0].cpk[0] == 48);
    REQUIRE(result.atoms[0].cpk[1] == 80);
    REQUIRE(result.atoms[0].cpk[2] == 248);
    REQUIRE(result.atoms[1].cpk[0] == 144);
    REQUIRE(result.atoms[1].cpk[1] == 144);
    REQUIRE(result.atoms[1].cpk[2] == 144);
    REQUIRE(PDBLoader::cpkTable().size() == 118);
}

TEST_CASE("PDBLoader deduplicates bonds and stores them zero based") {
    PDBLoader loader;
    const auto result = loader.parse(kPdb);
    // The repeated CONECT 1 2 is deduplicated, so only two bonds remain.
    REQUIRE(result.bonds.size() == 2);
    REQUIRE(result.bonds[0][0] == 0);
    REQUIRE(result.bonds[0][1] == 1);
    REQUIRE(result.bonds[0][2] == 1);
    REQUIRE(result.bonds[1][0] == 1);
    REQUIRE(result.bonds[1][1] == 2);
}

TEST_CASE("PDBLoader falls back to the atom name columns when the element is blank") {
    PDBLoader loader;
    const std::string line =
        "ATOM      1  N   ALA A   1      11.104  13.207  10.000  1.00  0.00\n";
    const auto result = loader.parse(line);
    REQUIRE(result.atoms.size() == 1);
    // Columns 76 to 78 are empty here, so the name columns are used instead.
    REQUIRE(result.atoms[0].symbol == "n");
}
