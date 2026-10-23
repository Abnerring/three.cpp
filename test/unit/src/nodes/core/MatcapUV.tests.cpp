#include "nodes/core/MatcapUV.h"

#include "catch.hpp"

#include <string>

TEST_CASE("MatcapUV scales by a little under a half") {
    // The scale is not a half, which is the whole point: the coordinates stop just short of the
    // edges so that an undersized disk does not show its rim.
    REQUIRE(MatcapUV::kScale == 0.495);
    REQUIRE(MatcapUV::kScale != 0.5);
    REQUIRE(MatcapUV::kScale < 0.5);
    REQUIRE(MatcapUV::kOffset == 0.5);
    // The range is therefore not nought to one either.
    REQUIRE(MatcapUV::kMinimum == 0.005);
    REQUIRE(MatcapUV::kMaximum == 0.995);
    REQUIRE(MatcapUV::kMaximum < 1.0);
}

TEST_CASE("MatcapUV puts a dot product into that range") {
    // The ends of the dot product's range become the ends of the coordinate range.
    REQUIRE(MatcapUV::coordinate(-1.0) == Approx(MatcapUV::kMinimum).epsilon(1e-12));
    REQUIRE(MatcapUV::coordinate(1.0) == Approx(MatcapUV::kMaximum).epsilon(1e-12));
    // The middle stays in the middle, since the offset is a half.
    REQUIRE(MatcapUV::coordinate(0.0) == 0.5);
    // A half scale would have given nought and one, which the two comparisons above separate.
    REQUIRE(MatcapUV::coordinate(1.0) != 1.0);
    REQUIRE(MatcapUV::coordinate(-1.0) != 0.0);
}

TEST_CASE("MatcapUV builds its first axis from a swapped and negated pair") {
    // The first part is the third of the view direction and the third is the negated first; the
    // middle is nought.
    const auto axis = MatcapUV::firstAxis(2.0, 3.0);
    REQUIRE(axis.x == 3.0);
    REQUIRE(axis.y == 0.0);
    REQUIRE(axis.z == -2.0);
    // The middle is always nought, whatever the two parts are.
    REQUIRE(MatcapUV::firstAxis(0.0, 0.0).y == 0.0);
    // The two are not simply copied, so the answer differs from the input.
    REQUIRE(axis.x != 2.0);
    REQUIRE(axis.z != 3.0);
    // A nought in either place stays nought rather than becoming minus nought in a way that
    // matters.
    const auto zero = MatcapUV::firstAxis(0.0, 5.0);
    REQUIRE(zero.x == 5.0);
    REQUIRE(zero.z == 0.0);
}

TEST_CASE("MatcapUV names the timer types") {
    // Two of the three timer uniforms are floats and the frame counter is an unsigned integer,
    // as the camera index is.
    REQUIRE(std::string(MatcapUV::kTimeType) == "float");
    REQUIRE(std::string(MatcapUV::kDeltaTimeType) == "float");
    REQUIRE(std::string(MatcapUV::kFrameIdType) == "uint");
    REQUIRE(std::string(MatcapUV::kFrameIdType) != std::string(MatcapUV::kTimeType));
}