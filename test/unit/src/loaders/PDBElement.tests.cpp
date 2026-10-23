#include "loaders/PDBLoader.h"

#include "catch.hpp"

#include <array>
#include <string>

namespace {
// A line whose two element columns sit where the format puts them, with the earlier pair empty.
std::string lineWithLateElement(const std::string& element) {
    std::string line(76, ' ');
    line += element;
    return line;
}
// A line with the late columns blank and the earlier pair filled instead.
std::string lineWithEarlyElement(const std::string& element) {
    std::string line(12, ' ');
    line += element;
    line += std::string(60, ' ');
    return line;
}
}  // namespace

TEST_CASE("PDBLoader reads the element from the later columns") {
    // The two columns near the end are the ordinary place for it.
    REQUIRE(PDBLoader::elementOf(lineWithLateElement(" N")) == "n");
    REQUIRE(PDBLoader::elementOf(lineWithLateElement("FE")) == "fe");
    // The answer is lower cased whatever the file wrote.
    REQUIRE(PDBLoader::elementOf(lineWithLateElement("He")) == "he");
}

TEST_CASE("PDBLoader falls back to the earlier columns") {
    // When the later pair is blank the earlier one is used.
    REQUIRE(PDBLoader::elementOf(lineWithEarlyElement(" C")) == "c");
    // Both blank gives nothing rather than a space.
    REQUIRE(PDBLoader::elementOf(lineWithLateElement("  ")).empty());
    REQUIRE(PDBLoader::elementOf("").empty());
    // A line too short to reach the earlier pair gives nothing as well.
    REQUIRE(PDBLoader::elementOf("ATOM").empty());
}

TEST_CASE("PDBLoader reads the four bond columns") {
    // Four partners at fixed offsets five apart.
    const auto& columns = PDBLoader::bondColumns();
    REQUIRE(columns.size() == 4);
    REQUIRE(columns[0] == 11);
    REQUIRE(columns[1] == 16);
    REQUIRE(columns[2] == 21);
    REQUIRE(columns[3] == 26);
    // Each is five apart, which is what makes a line read as four pairs.
    for (std::size_t i = 1; i < columns.size(); ++i) {
        REQUIRE(columns[i] - columns[i - 1] == 5);
    }
}

TEST_CASE("PDBLoader numbers atoms from zero") {
    // The format numbers from one, so the first atom becomes zero.
    REQUIRE(PDBLoader::atomIndex(1) == 0);
    REQUIRE(PDBLoader::atomIndex(5) == 4);
    REQUIRE(PDBLoader::atomIndex(0) == -1);
}