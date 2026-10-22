#include "loaders/BVHLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
const char* kMotion =
    "MOTION\n"
    "Frames: 2\n"
    "Frame Time: 0.033\n"
    "0 0 0 0 0 0\n"
    "1 2 3 4 5 6\n";
}

TEST_CASE("BVHLoader reads the motion header and every frame") {
    const auto lines = BVHLoader::splitLines(kMotion);
    std::size_t cursor = 0;
    const auto motion = BVHLoader::parseMotion(lines, cursor, 6);
    REQUIRE(motion.numFrames == 2);
    REQUIRE(motion.frameTime == Approx(0.033).epsilon(1e-12));
    // One time per frame, index times the duration.
    REQUIRE(motion.times.size() == 2);
    REQUIRE(motion.times[0] == 0.0);
    REQUIRE(motion.times[1] == Approx(0.033).epsilon(1e-12));
    // Six values per frame, row major.
    REQUIRE(motion.values.size() == 12);
    REQUIRE(motion.values[0] == 0.0);
    REQUIRE(motion.values[6] == 1.0);
    REQUIRE(motion.values[11] == 6.0);
    REQUIRE(cursor == 5);
}

TEST_CASE("BVHLoader rejects a missing motion keyword") {
    const auto lines = BVHLoader::splitLines("Frames: 1\nFrame Time: 0.1\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseMotion(lines, cursor, 3), "THREE.BVHLoader: MOTION expected.");
}

TEST_CASE("BVHLoader rejects a malformed frame count") {
    const auto lines = BVHLoader::splitLines("MOTION\nFrames\nFrame Time: 0.1\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseMotion(lines, cursor, 3),
                        "THREE.BVHLoader: Failed to read number of frames.");
}

TEST_CASE("BVHLoader rejects a malformed frame time") {
    const auto lines = BVHLoader::splitLines("MOTION\nFrames: 1\nTime: 0.1\n0 0 0\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseMotion(lines, cursor, 3),
                        "THREE.BVHLoader: Failed to read frame time.");
}

TEST_CASE("BVHLoader rejects a frame with too few values") {
    const auto lines = BVHLoader::splitLines("MOTION\nFrames: 1\nFrame Time: 0.1\n1 2\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseMotion(lines, cursor, 3),
                        "THREE.BVHLoader: Failed to read frame data.");
}

TEST_CASE("BVHLoader handles zero frames") {
    const auto lines = BVHLoader::splitLines("MOTION\nFrames: 0\nFrame Time: 0.5\n");
    std::size_t cursor = 0;
    const auto motion = BVHLoader::parseMotion(lines, cursor, 3);
    REQUIRE(motion.numFrames == 0);
    REQUIRE(motion.times.empty());
    REQUIRE(motion.values.empty());
}
