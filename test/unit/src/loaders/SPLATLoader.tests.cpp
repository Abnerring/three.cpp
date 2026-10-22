#include "loaders/GaussianSplatUtils.h"
#include "loaders/SPLATLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void pushFloat32LE(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    out.push_back(std::uint8_t(bits & 0xff));
    out.push_back(std::uint8_t((bits >> 8) & 0xff));
    out.push_back(std::uint8_t((bits >> 16) & 0xff));
    out.push_back(std::uint8_t((bits >> 24) & 0xff));
}

void pushRow(std::vector<std::uint8_t>& out, float cx, float cy, float cz, float sx, float sy,
             float sz, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a,
             std::uint8_t qw, std::uint8_t qx, std::uint8_t qy, std::uint8_t qz) {
    pushFloat32LE(out, cx);
    pushFloat32LE(out, cy);
    pushFloat32LE(out, cz);
    pushFloat32LE(out, sx);
    pushFloat32LE(out, sy);
    pushFloat32LE(out, sz);
    out.push_back(r);
    out.push_back(g);
    out.push_back(b);
    out.push_back(a);
    out.push_back(qw);
    out.push_back(qx);
    out.push_back(qy);
    out.push_back(qz);
}
}  // namespace

// The expected covariance entries come from running the pinned GaussianSplatUtils in node.
TEST_CASE("writeCovariance squares the scale for an identity rotation") {
    float covariance[6] = {0, 0, 0, 0, 0, 0};
    GaussianSplatUtils::writeCovariance(covariance, 0, 2, 3, 4, 0, 0, 0, 1);
    REQUIRE(covariance[0] == Approx(4.0).epsilon(1e-6));
    REQUIRE(covariance[1] == 0.0f);
    REQUIRE(covariance[2] == 0.0f);
    REQUIRE(covariance[3] == Approx(9.0).epsilon(1e-6));
    REQUIRE(covariance[4] == 0.0f);
    REQUIRE(covariance[5] == Approx(16.0).epsilon(1e-6));
}

TEST_CASE("writeCovariance keeps a unit scale unit for a forty five degree rotation") {
    float covariance[6] = {0, 0, 0, 0, 0, 0};
    const double half = std::sqrt(0.5);
    GaussianSplatUtils::writeCovariance(covariance, 0, 1, 1, 1, 0, 0, half, half);
    REQUIRE(covariance[0] == Approx(1.0).epsilon(1e-6));
    REQUIRE(covariance[1] == 0.0f);
    REQUIRE(covariance[2] == 0.0f);
    REQUIRE(covariance[3] == Approx(1.0).epsilon(1e-6));
    REQUIRE(covariance[4] == 0.0f);
    REQUIRE(covariance[5] == Approx(1.0).epsilon(1e-6));
}

TEST_CASE("the splat colour helpers match upstream") {
    REQUIRE(GaussianSplatUtils::sigmoid(0) == 0.5);
    REQUIRE(GaussianSplatUtils::sigmoid(2) == Approx(0.880797077978).epsilon(1e-12));
    REQUIRE(GaussianSplatUtils::sh0ToLinear(0) == 0.5);
    REQUIRE(GaussianSplatUtils::linearToSH0(0.5) == 0.0);
    REQUIRE(GaussianSplatUtils::SH_C0 == Approx(0.2820947917738781).epsilon(1e-15));
}

TEST_CASE("SPLATLoader reads little endian centres, colours and scales") {
    std::vector<std::uint8_t> buffer;
    pushRow(buffer, 1, 2, 3, 2, 3, 4, 10, 20, 30, 40, 255, 128, 128, 128);
    pushRow(buffer, -1, -2, -3, 1, 1, 1, 50, 60, 70, 80, 255, 128, 128, 128);
    REQUIRE(buffer.size() == 64);

    SPLATLoader loader;
    const auto result = loader.parse(buffer);
    REQUIRE(result.centers.size() == 6);
    REQUIRE(result.centers[0] == 1.0f);
    REQUIRE(result.centers[1] == 2.0f);
    REQUIRE(result.centers[2] == 3.0f);
    REQUIRE(result.centers[3] == -1.0f);

    REQUIRE(result.colors.size() == 8);
    REQUIRE(result.colors[0] == 10);
    REQUIRE(result.colors[1] == 20);
    REQUIRE(result.colors[2] == 30);
    REQUIRE(result.colors[3] == 40);

    // The first splat has scale (2,3,4) and an almost identity rotation, so its covariance is
    // close to the squared scales but not exactly equal because the quaternion is normalised.
    REQUIRE(result.covariances.size() == 12);
    REQUIRE(result.covariances[0] > 3.9f);
}

TEST_CASE("SPLATLoader rejects a byte length that is not a whole number of rows") {
    SPLATLoader loader;
    std::vector<std::uint8_t> bad(33, 0);
    REQUIRE_THROWS_WITH(loader.parse(bad), "THREE.SPLATLoader: Invalid .splat byte length.");
}
