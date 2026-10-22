#include "loaders/BVHLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("BVHLoader writes position channels to their axes") {
    const std::vector<std::string> channels{"Xposition", "Yposition", "Zposition"};
    const auto keyframe = BVHLoader::decodeChannels(channels, {1, 2, 3});
    REQUIRE(keyframe.position[0] == 1.0);
    REQUIRE(keyframe.position[1] == 2.0);
    REQUIRE(keyframe.position[2] == 3.0);
    // No rotation channel means an identity orientation.
    REQUIRE(keyframe.rotation[0] == 0.0);
    REQUIRE(keyframe.rotation[3] == 1.0);
}

TEST_CASE("BVHLoader treats rotations as degrees") {
    const auto keyframe = BVHLoader::decodeChannels({"Zrotation"}, {90});
    // Ninety degrees about Z is the familiar half angle quaternion.
    REQUIRE(keyframe.rotation[0] == Approx(0.0).margin(1e-9));
    REQUIRE(keyframe.rotation[1] == Approx(0.0).margin(1e-9));
    REQUIRE(keyframe.rotation[2] == Approx(0.7071067811865476).epsilon(1e-9));
    REQUIRE(keyframe.rotation[3] == Approx(0.7071067811865476).epsilon(1e-9));
}

TEST_CASE("BVHLoader composes rotations in declaration order") {
    // Ninety about Z then ninety about X composes to (0.5, 0.5, 0.5, 0.5), which a wrong
    // order or an overwrite instead of a compose would not produce.
    const auto keyframe = BVHLoader::decodeChannels({"Zrotation", "Xrotation"}, {90, 90});
    REQUIRE(keyframe.rotation[0] == Approx(0.5).epsilon(1e-9));
    REQUIRE(keyframe.rotation[1] == Approx(0.5).epsilon(1e-9));
    REQUIRE(keyframe.rotation[2] == Approx(0.5).epsilon(1e-9));
    REQUIRE(keyframe.rotation[3] == Approx(0.5).epsilon(1e-9));
}

TEST_CASE("BVHLoader mixes position and rotation channels") {
    const std::vector<std::string> channels{"Xposition", "Zrotation", "Yposition"};
    const auto keyframe = BVHLoader::decodeChannels(channels, {4, 180, 5});
    REQUIRE(keyframe.position[0] == 4.0);
    REQUIRE(keyframe.position[1] == 5.0);
    // Only Z was rotated, and a full turn is a quaternion with w of minus one.
    REQUIRE(keyframe.rotation[2] == Approx(1.0).epsilon(1e-9));
    REQUIRE(keyframe.rotation[3] == Approx(0.0).margin(1e-9));
}

TEST_CASE("BVHLoader ignores unknown channel names") {
    const auto keyframe = BVHLoader::decodeChannels({"nonsense"}, {5});
    REQUIRE(keyframe.position[0] == 0.0);
    REQUIRE(keyframe.rotation[3] == 1.0);
}

TEST_CASE("BVHLoader stops when the values run out") {
    const std::vector<std::string> channels{"Xposition", "Yposition"};
    const auto keyframe = BVHLoader::decodeChannels(channels, {7});
    REQUIRE(keyframe.position[0] == 7.0);
    REQUIRE(keyframe.position[1] == 0.0);
}
