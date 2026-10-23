#include "nodes/core/EquirectUV.h"

#include "catch.hpp"

#include <cmath>

TEST_CASE("EquirectUV puts a direction at the middle of the texture when it points along x") {
    // A direction along the first axis gives a horizontal coordinate of a half, since the angle
    // is nought, and a vertical one of a half, since the second part is nought.
    const auto mid = EquirectUV::toCoordinates(1.0, 0.0, 0.0);
    REQUIRE(mid.u == Approx(0.5).epsilon(1e-12));
    REQUIRE(mid.v == Approx(0.5).epsilon(1e-12));
}

TEST_CASE("EquirectUV uses the third part as the angle's first argument") {
    // A direction along the third axis gives a quarter turn, so the horizontal coordinate is a
    // quarter of the way round rather than a half. Swapping the two arguments would give the
    // same answer here, so the next case separates them.
    const auto alongZ = EquirectUV::toCoordinates(0.0, 0.0, 1.0);
    REQUIRE(alongZ.u == Approx(0.75).epsilon(1e-12));
    // A direction along the first axis has already been shown to be a half; together the two
    // show the angle is measured from the first axis towards the third.
    const auto alongX = EquirectUV::toCoordinates(1.0, 0.0, 0.0);
    REQUIRE(alongX.u == Approx(0.5).epsilon(1e-12));
    REQUIRE(alongZ.u != alongX.u);
}

TEST_CASE("EquirectUV clamps the vertical part before its angle") {
    // The angle's inverse accepts only the range minus one to one, so a part that is longer is
    // clamped first and gives an angle rather than a not a number.
    REQUIRE(EquirectUV::clampForAngle(2.0) == 1.0);
    REQUIRE(EquirectUV::clampForAngle(-2.0) == -1.0);
    REQUIRE(EquirectUV::clampForAngle(0.25) == 0.25);
    // A direction longer than one, which a world direction may be if it was never normalised,
    // still gives finite coordinates.
    const auto long_ = EquirectUV::toCoordinates(0.0, 3.0, 0.0);
    REQUIRE(std::isfinite(long_.v));
    REQUIRE_FALSE(std::isnan(long_.v));
    REQUIRE(long_.v == Approx(1.0).epsilon(1e-12));
}

TEST_CASE("EquirectUV's horizontal coordinate needs no clamp") {
    // The angle it comes from accepts every input, so unlike the vertical one it is never
    // clamped and a long direction still gives a sensible answer.
    const auto long_ = EquirectUV::toCoordinates(5.0, 0.0, 5.0);
    REQUIRE(std::isfinite(long_.u));
    REQUIRE_FALSE(std::isnan(long_.u));
    // It stays inside the range because the angle is periodic, not because of a clamp.
    REQUIRE(long_.u >= 0.0);
    REQUIRE(long_.u <= 1.0);
}

TEST_CASE("EquirectUV's two functions undo one another") {
    // A direction taken to coordinates and back gives the direction again, up to rounding.
    const auto direction = EquirectUV::toDirection(0.3, 0.7);
    const auto back = EquirectUV::toCoordinates(direction.x, direction.y, direction.z);
    REQUIRE(back.u == Approx(0.3).epsilon(1e-9));
    REQUIRE(back.v == Approx(0.7).epsilon(1e-9));
    // The direction is of unit length, which is what the sines and cosines give.
    const double length =
        std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    REQUIRE(length == Approx(1.0).epsilon(1e-12));
    // The poles map to the ends of the vertical range.
    const auto top = EquirectUV::toDirection(0.5, 1.0);
    REQUIRE(top.y == Approx(1.0).epsilon(1e-12));
    const auto bottom = EquirectUV::toDirection(0.5, 0.0);
    REQUIRE(bottom.y == Approx(-1.0).epsilon(1e-12));
}