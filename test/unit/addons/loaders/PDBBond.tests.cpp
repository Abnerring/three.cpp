#include "loaders/PDBLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("PDBLoader trims both ends") {
    // Spaces on either side go and the middle stays.
    REQUIRE(PDBLoader::trimmed("  N  ") == "N");
    REQUIRE(PDBLoader::trimmed("N") == "N");
    REQUIRE(PDBLoader::trimmed("   ") == "");
    REQUIRE(PDBLoader::trimmed("") == "");
    // Spaces inside are kept.
    REQUIRE(PDBLoader::trimmed(" a b ") == "a b");
}

TEST_CASE("PDBLoader capitalises by raising the first and lowering the rest") {
    // One letter names become a single capital.
    REQUIRE(PDBLoader::capitalised("n") == "N");
    REQUIRE(PDBLoader::capitalised("N") == "N");
    // Two letters get the first raised and the second lowered, so the result is not NO.
    REQUIRE(PDBLoader::capitalised("NO") == "No");
    REQUIRE(PDBLoader::capitalised("no") == "No");
    REQUIRE(PDBLoader::capitalised("fe") == "Fe");
    // An empty name stays empty rather than reaching past its first character.
    REQUIRE(PDBLoader::capitalised("").empty());
}

TEST_CASE("PDBLoader hashes a bond the same way from either end") {
    // The key puts the smaller number first.
    REQUIRE(PDBLoader::bondHash(1, 2) == "s1e2");
    REQUIRE(PDBLoader::bondHash(2, 1) == "s1e2");
    // So the two orders agree, which is what stops a bond being filed twice.
    REQUIRE(PDBLoader::bondHash(7, 3) == PDBLoader::bondHash(3, 7));
    // Equal numbers give the same one twice.
    REQUIRE(PDBLoader::bondHash(5, 5) == "s5e5");
}

TEST_CASE("PDBLoader drops a bond that names nothing or is already filed") {
    // A blank column reads as zero and names no atom.
    REQUIRE_FALSE(PDBLoader::acceptsBond(0, false));
    // A new partner is filed.
    REQUIRE(PDBLoader::acceptsBond(3, false));
    // One already filed is not counted again.
    REQUIRE_FALSE(PDBLoader::acceptsBond(3, true));
    // A zero is refused whether or not its key was seen.
    REQUIRE_FALSE(PDBLoader::acceptsBond(0, true));
}