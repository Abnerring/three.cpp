#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("LDrawLoader builds a whole reference") {
    // The directive is already read, so the line starts at the colour.
    LDrawLoader::LineParser parser("16 1 2 3 1 0 0 0 1 0 0 0 1 s/stud.dat", 9);
    LDrawLoader::BfcState state;
    bool startingStep = false;
    const auto reference =
        LDrawLoader::buildReference(parser, state, startingStep, {}, {});
    REQUIRE(reference.colorCode == "16");
    // The position is the fourth column of the column major matrix.
    REQUIRE(reference.matrix[12] == 1.0);
    REQUIRE(reference.matrix[13] == 2.0);
    REQUIRE(reference.matrix[14] == 3.0);
    // The name was tidied and then put under the standard folder.
    REQUIRE(reference.fileName == "parts/s/stud.dat");
    // No flags were pending.
    REQUIRE_FALSE(reference.inverted);
    REQUIRE_FALSE(reference.startingBuildingStep);
}

TEST_CASE("LDrawLoader takes backslashes out of a reference name") {
    // A name written with the other separator is tidied before it is resolved.
    LDrawLoader::LineParser parser("4 0 0 0 1 0 0 0 1 0 0 0 1 sub\\part.dat", 3);
    LDrawLoader::BfcState state;
    bool startingStep = false;
    const auto reference = LDrawLoader::buildReference(parser, state, startingStep, {}, {});
    REQUIRE(reference.fileName == "sub/part.dat");
}

TEST_CASE("LDrawLoader applies a mapping to a built reference") {
    // A mapped name wins over the standard folders, and the mapping is matched after tidying.
    LDrawLoader::LineParser parser("4 0 0 0 1 0 0 0 1 0 0 0 1 s/stud.dat", 3);
    LDrawLoader::BfcState state;
    bool startingStep = false;
    const std::vector<std::string> names{"s/stud.dat"};
    const std::vector<std::string> paths{"custom/stud.dat"};
    const auto reference = LDrawLoader::buildReference(parser, state, startingStep, names, paths);
    REQUIRE(reference.fileName == "custom/stud.dat");
}

TEST_CASE("LDrawLoader gives a built reference the pending flags") {
    // Both flags are set before the line is read.
    LDrawLoader::BfcState state;
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    bool startingStep = true;
    LDrawLoader::LineParser parser("16 0 0 0 1 0 0 0 1 0 0 0 1 a.dat", 1);
    const auto reference = LDrawLoader::buildReference(parser, state, startingStep, {}, {});
    REQUIRE(reference.inverted);
    REQUIRE(reference.startingBuildingStep);
}

TEST_CASE("LDrawLoader clears the flags after building a reference") {
    // The state is quiet for the next line.
    LDrawLoader::BfcState state;
    REQUIRE(LDrawLoader::applyBfcToken("INVERTNEXT", state));
    bool startingStep = true;
    LDrawLoader::LineParser first("16 0 0 0 1 0 0 0 1 0 0 0 1 a.dat", 1);
    LDrawLoader::buildReference(first, state, startingStep, {}, {});
    REQUIRE_FALSE(state.inverted);
    REQUIRE_FALSE(startingStep);

    // The reference after it carries nothing.
    LDrawLoader::LineParser second("16 0 0 0 1 0 0 0 1 0 0 0 1 b.dat", 2);
    const auto secondReference = LDrawLoader::buildReference(second, state, startingStep, {}, {});
    REQUIRE_FALSE(secondReference.inverted);
    REQUIRE_FALSE(secondReference.startingBuildingStep);
}

TEST_CASE("LDrawLoader leaves the cursor at the end of a reference") {
    LDrawLoader::LineParser parser("16 0 0 0 1 0 0 0 1 0 0 0 1 a.dat", 1);
    LDrawLoader::BfcState state;
    bool startingStep = false;
    LDrawLoader::buildReference(parser, state, startingStep, {}, {});
    // Every token of the line was consumed.
    REQUIRE(parser.isAtTheEnd());
}