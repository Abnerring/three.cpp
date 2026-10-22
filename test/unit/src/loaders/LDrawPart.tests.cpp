#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("LDrawLoader tidies a part file name") {
    // The whitespace is not part of the name.
    REQUIRE(LDrawLoader::normalizeFileName("  3001.dat  ") == "3001.dat");
    // Backslashes become forward slashes.
    REQUIRE(LDrawLoader::normalizeFileName("parts\\3001.dat") == "parts/3001.dat");
    REQUIRE(LDrawLoader::normalizeFileName("a\\b\\c.dat") == "a/b/c.dat");
    // A name already using forward slashes is unchanged.
    REQUIRE(LDrawLoader::normalizeFileName("parts/3001.dat") == "parts/3001.dat");
    // Nothing at all stays nothing.
    REQUIRE(LDrawLoader::normalizeFileName("").empty());
}

TEST_CASE("LDrawLoader finds a part in the standard folders") {
    // A name under s lives under parts, and the folder stays in the path.
    REQUIRE(LDrawLoader::resolveFileName("s/stud.dat", {}, {}) == "parts/s/stud.dat");
    // A name under 48 lives under p.
    REQUIRE(LDrawLoader::resolveFileName("48/stud.dat", {}, {}) == "p/48/stud.dat");
    // Anything else is left alone.
    REQUIRE(LDrawLoader::resolveFileName("3001.dat", {}, {}) == "3001.dat");
    REQUIRE(LDrawLoader::resolveFileName("parts/3001.dat", {}, {}) == "parts/3001.dat");
}

TEST_CASE("LDrawLoader prefers the mapping the caller gave") {
    // A mapped name uses the path as given, even when it looks like a standard folder.
    REQUIRE(LDrawLoader::resolveFileName("s/stud.dat", {"s/stud.dat"}, {"custom/stud.dat"}) ==
            "custom/stud.dat");
    // A name not in the mapping falls through to the standard folders.
    REQUIRE(LDrawLoader::resolveFileName("s/other.dat", {"s/stud.dat"}, {"custom/stud.dat"}) ==
            "parts/s/other.dat");
    // The two lists pair up by position, so the second name takes the second path.
    const std::vector<std::string> names{"a.dat", "s/b.dat"};
    const std::vector<std::string> paths{"/x/a.dat", "/y/b.dat"};
    REQUIRE(LDrawLoader::resolveFileName("s/b.dat", names, paths) == "/y/b.dat");
}

TEST_CASE("LDrawLoader places a reference with its matrix") {
    LDrawLoader::ReferenceLine reference;
    // A rotation that identifies, and a position of one, two, three.
    reference.rotation = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    reference.position = {1, 2, 3};
    const auto matrix = LDrawLoader::referenceMatrix(reference);
    // The rotation is stored column major, which for an identity is the same either way.
    REQUIRE(matrix[0] == 1.0);
    REQUIRE(matrix[5] == 1.0);
    REQUIRE(matrix[10] == 1.0);
    // The position lands in the fourth column.
    REQUIRE(matrix[12] == 1.0);
    REQUIRE(matrix[13] == 2.0);
    REQUIRE(matrix[14] == 3.0);
    REQUIRE(matrix[15] == 1.0);
    // The padding of the fourth column of the rotation is zero.
    REQUIRE(matrix[3] == 0.0);
    REQUIRE(matrix[7] == 0.0);
    REQUIRE(matrix[11] == 0.0);
}

TEST_CASE("LDrawLoader transposes a non symmetric rotation") {
    // A rotation whose rows differ from its columns.
    LDrawLoader::ReferenceLine reference;
    reference.rotation = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    const auto matrix = LDrawLoader::referenceMatrix(reference);
    // The first stored column takes the first element of each row.
    REQUIRE(matrix[0] == 1.0);
    REQUIRE(matrix[1] == 4.0);
    REQUIRE(matrix[2] == 7.0);
    // The second column takes the second element of each row.
    REQUIRE(matrix[4] == 2.0);
    REQUIRE(matrix[5] == 5.0);
    REQUIRE(matrix[6] == 8.0);
}
