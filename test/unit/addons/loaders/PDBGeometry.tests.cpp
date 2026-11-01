#include "loaders/PDBLoader.h"

#include "catch.hpp"

#include <array>
#include <vector>

TEST_CASE("PDBLoader takes atom colours out of the sRGB space") {
    // White and black survive the conversion unchanged, since both ends of the curve are fixed.
    const auto white = PDBLoader::atomColour(255, 255, 255);
    REQUIRE(white.r == Approx(1.0).epsilon(1e-9));
    REQUIRE(white.g == Approx(1.0).epsilon(1e-9));
    REQUIRE(white.b == Approx(1.0).epsilon(1e-9));
    const auto black = PDBLoader::atomColour(0, 0, 0);
    REQUIRE(black.r == 0.0);
    REQUIRE(black.g == 0.0);
    REQUIRE(black.b == 0.0);
    // A mid grey comes back darker than the plain quotient, which is what shows the conversion
    // happened at all.
    const auto grey = PDBLoader::atomColour(128, 128, 128);
    REQUIRE(grey.r < 128.0 / 255.0);
    REQUIRE(grey.r > 0.0);
    // The three channels of a grey stay equal.
    REQUIRE(grey.r == grey.g);
    REQUIRE(grey.g == grey.b);
}

TEST_CASE("PDBLoader colours a list of atoms") {
    std::vector<PDBLoader::Atom> atoms;
    PDBLoader::Atom carbon;
    carbon.x = 0;
    carbon.element = "c";
    atoms.push_back(carbon);
    PDBLoader::Atom unknown;
    unknown.x = 1;
    unknown.element = "zzz";
    atoms.push_back(unknown);
    const auto colours = PDBLoader::atomColours(atoms);
    REQUIRE(colours.size() == 2);
    // Carbon's colour comes from the table, and is not white.
    REQUIRE(colours[0].r != 1.0);
    // An element the table does not know is black rather than uninitialised.
    REQUIRE(colours[1].r == 0.0);
    REQUIRE(colours[1].g == 0.0);
    REQUIRE(colours[1].b == 0.0);
}

TEST_CASE("PDBLoader lists both ends of every bond") {
    std::vector<PDBLoader::Atom> atoms(2);
    atoms[0].x = 1;
    atoms[0].y = 2;
    atoms[0].z = 3;
    atoms[1].x = 4;
    atoms[1].y = 5;
    atoms[1].z = 6;
    const std::vector<std::array<int, 3>> bonds{{0, 1, 1}};
    const auto positions = PDBLoader::bondPositions(atoms, bonds);
    REQUIRE(positions.size() == 6);
    // The first end is the start atom and the second the end one.
    REQUIRE(positions[0] == 1);
    REQUIRE(positions[2] == 3);
    REQUIRE(positions[3] == 4);
    REQUIRE(positions[5] == 6);
    // Two bonds give twelve numbers.
    const auto two = PDBLoader::bondPositions(atoms, {{0, 1, 1}, {1, 0, 1}});
    REQUIRE(two.size() == 12);
    REQUIRE(two[6] == 4);
}

TEST_CASE("PDBLoader skips a bond that names no atom") {
    std::vector<PDBLoader::Atom> atoms(1);
    atoms[0].x = 7;
    // An end past the list is skipped rather than read past it.
    const auto beyond = PDBLoader::bondPositions(atoms, {{0, 5, 1}});
    REQUIRE(beyond.empty());
    // So is a negative one.
    const auto negative = PDBLoader::bondPositions(atoms, {{-1, 0, 1}});
    REQUIRE(negative.empty());
    // A bond between an atom and itself is kept, since the format can say that.
    const auto self = PDBLoader::bondPositions(atoms, {{0, 0, 1}});
    REQUIRE(self.size() == 6);
    REQUIRE(self[0] == 7);
}