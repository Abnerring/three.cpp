#include "loaders/IESLoader.h"

#include "catch.hpp"

#include <vector>

namespace {
IESLoader::Tables makeTables() {
    IESLoader::Tables tables;
    tables.verticalAngles = {0, 90};
    tables.horizontalAngles = {0, 45};
    // Row per horizontal angle, column per vertical angle.
    tables.candela = {{1, 3}, {5, 7}};
    return tables;
}
}  // namespace

TEST_CASE("IESLoader interpolates between four readings") {
    const auto tables = makeTables();
    // The middle of both ranges is the average of the four corners.
    const double v = IESLoader::interpolateCandela(tables, 45, 22.5);
    REQUIRE(v == Approx(4.0).epsilon(1e-12));
}

TEST_CASE("IESLoader returns the corner readings at the corners") {
    const auto tables = makeTables();
    REQUIRE(IESLoader::interpolateCandela(tables, 0, 0) == Approx(1.0).epsilon(1e-12));
    REQUIRE(IESLoader::interpolateCandela(tables, 90, 45) == Approx(7.0).epsilon(1e-12));
}

TEST_CASE("IESLoader mirrors a horizontal angle past the measured end") {
    // The measured range ends at forty five, so a sample at sixty folds back to thirty.
    REQUIRE(IESLoader::mirrorHorizontalAngle(60, 45) == Approx(30.0).epsilon(1e-12));
    // Inside the range nothing changes.
    REQUIRE(IESLoader::mirrorHorizontalAngle(20, 45) == 20.0);
    // A full cycle lamp has no end to mirror around.
    REQUIRE(IESLoader::mirrorHorizontalAngle(120, 0) == 120.0);
    // Beyond a full cycle it wraps first.
    REQUIRE(IESLoader::mirrorHorizontalAngle(200, 45) == Approx(20.0).epsilon(1e-12));
}

TEST_CASE("IESLoader lays the samples out by column") {
    const auto tables = makeTables();
    const auto values = IESLoader::sampleValues(tables, 2);
    // Three hundred and sixty columns of one hundred and eighty rows.
    REQUIRE(values.size() == 360 * 180);
    // The first sample is the first corner.
    REQUIRE(values[0] == Approx(1.0).epsilon(1e-9));
    // Row one of column one is at index one times the height.
    const double atSecondRow = values[180];
    REQUIRE(atSecondRow > 0.0);
}

TEST_CASE("IESLoader uses a single column for one horizontal angle") {
    IESLoader::Tables tables;
    tables.verticalAngles = {0, 90};
    tables.horizontalAngles = {0};
    tables.candela = {{2, 4}};
    const auto values = IESLoader::sampleValues(tables, 1);
    // One column of one hundred and eighty rows.
    REQUIRE(values.size() == 180);
    REQUIRE(values[0] == Approx(2.0).epsilon(1e-9));
}