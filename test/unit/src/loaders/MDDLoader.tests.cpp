#include "loaders/MDDLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void pushUint32BE(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(std::uint8_t((value >> 24) & 0xff));
    out.push_back(std::uint8_t((value >> 16) & 0xff));
    out.push_back(std::uint8_t((value >> 8) & 0xff));
    out.push_back(std::uint8_t(value & 0xff));
}

void pushFloat32BE(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    pushUint32BE(out, bits);
}

// Two frames, three points, frame times 0 and 0.5, then the vertex data.
std::vector<std::uint8_t> makeMdd() {
    std::vector<std::uint8_t> data;
    pushUint32BE(data, 2);
    pushUint32BE(data, 3);
    pushFloat32BE(data, 0.0f);
    pushFloat32BE(data, 0.5f);
    for (float v = 0; v < 9; ++v) pushFloat32BE(data, v);
    pushFloat32BE(data, 1.0f);
    pushFloat32BE(data, 1.0f);
    pushFloat32BE(data, 1.0f);
    for (int i = 0; i < 6; ++i) pushFloat32BE(data, 2.0f);
    return data;
}
}  // namespace

TEST_CASE("MDDLoader reads the big endian header and vertex data") {
    MDDLoader loader;
    const auto result = loader.parse(makeMdd());
    REQUIRE(result.morphTargets.size() == 2);

    const auto& first = result.morphTargets[0];
    REQUIRE(first->name == "morph_0");
    REQUIRE(first->itemSize == 3);
    REQUIRE(first->count == 3);
    // A relative tolerance cannot express zero, so this compares exactly.
    REQUIRE(first->getX(0) == 0.0);
    // Frame zero holds the points (0,1,2), (3,4,5) and (6,7,8).
    REQUIRE(first->getY(1) == Approx(4.0).epsilon(1e-6));
    REQUIRE(first->getZ(2) == Approx(8.0).epsilon(1e-6));

    const auto& second = result.morphTargets[1];
    REQUIRE(second->name == "morph_1");
    // The second frame starts with the single one and continues with twos.
    REQUIRE(second->getX(0) == Approx(1.0).epsilon(1e-6));
}

TEST_CASE("MDDLoader builds a clip whose duration is the last frame time") {
    MDDLoader loader;
    const auto result = loader.parse(makeMdd());
    REQUIRE(result.clip != nullptr);
    // 0.5 is the last frame time, so that is the clip duration.
    REQUIRE(result.clip->duration == Approx(0.5).epsilon(1e-6));
    REQUIRE(result.clip->name.toString() == "default");
}

TEST_CASE("MDDLoader complains when the data ends early") {
    MDDLoader loader;
    std::vector<std::uint8_t> truncated{0, 0, 0, 2};
    REQUIRE_THROWS_WITH(loader.parse(truncated), "THREE.MDDLoader: Unexpected end of data");
}
