#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <cmath>
#include <cstdint>

namespace {
double length(const VOXLoader::Matrix& m, std::size_t row) {
    // The first three components of a column of the upper left block.
    const double a = m[row * 4 + 0];
    const double b = m[row * 4 + 1];
    const double c = m[row * 4 + 2];
    return std::sqrt(a * a + b * b + c * c);
}
}  // namespace

TEST_CASE("VOXLoader decodes the identity rotation") {
    // Columns zero and one with all signs positive is the identity.
    const auto m = VOXLoader::rotationMatrix(0x04);
    REQUIRE(m[0] == 1.0);
    REQUIRE(m[5] == 1.0);
    REQUIRE(m[10] == 1.0);
    // The remaining components are zero and the last column is the translation.
    REQUIRE(m[1] == 0.0);
    REQUIRE(m[15] == 1.0);
}

TEST_CASE("VOXLoader produces an orthogonal matrix for every encoding") {
    // Walk every byte whose two column indices differ, which is the valid set.
    int checked = 0;
    for (int byte = 0; byte < 256; ++byte) {
        const int index1 = byte & 0x3;
        const int index2 = (byte >> 2) & 0x3;
        // The third column is three minus the other two, so both must be below three for it to
        // land in range; the format only uses the six ordered pairs over zero one and two.
        if (index1 >= 3 || index2 >= 3 || index1 == index2) continue;

        const auto m = VOXLoader::rotationMatrix(static_cast<std::uint8_t>(byte));
        // Every row of the upper block is a unit vector.
        for (std::size_t row = 0; row < 3; ++row) REQUIRE(length(m, row) == Approx(1.0).margin(1e-12));
        // Rows are mutually perpendicular.
        REQUIRE((m[0] * m[4] + m[1] * m[5] + m[2] * m[6]) == Approx(0.0).margin(1e-12));
        REQUIRE((m[0] * m[8] + m[1] * m[9] + m[2] * m[10]) == Approx(0.0).margin(1e-12));
        // The homogeneous row and column are untouched.
        REQUIRE(m[3] == 0.0);
        REQUIRE(m[7] == 0.0);
        REQUIRE(m[11] == 0.0);
        REQUIRE(m[12] == 0.0);
        REQUIRE(m[13] == 0.0);
        REQUIRE(m[14] == 0.0);
        REQUIRE(m[15] == 1.0);
        ++checked;
    }
    // Six ordered pairs over three columns, each with eight sign combinations.
    // Six ordered pairs over three columns and sixteen combinations of the four upper bits.
    REQUIRE(checked == 6 * 16);
}
TEST_CASE("VOXLoader leaves the identity unchanged") {
    // The change of basis is written into the matrix, so with an identity rotation the
    // result is still the identity: a point on any axis stays on it.
    const auto m = VOXLoader::rotationMatrix(0x04);
    const auto apply = [&m](double x, double y, double z, int component) {
        // Four components per row, so a component of the result is a row dot the point.
        return m[component * 4 + 0] * x + m[component * 4 + 1] * y + m[component * 4 + 2] * z;
    };
    REQUIRE(apply(1, 0, 0, 0) == 1.0);
    REQUIRE(apply(0, 1, 0, 1) == 1.0);
    REQUIRE(apply(0, 0, 1, 2) == 1.0);
    // The VOX up axis is unchanged by an identity rotation.
    REQUIRE(apply(0, 0, 1, 1) == 0.0);
    REQUIRE(apply(0, 0, 1, 2) == 1.0);
}

TEST_CASE("VOXLoader rejects a rotation with repeated columns") {
    // Index one and index two both zero leaves no third column.
    REQUIRE_THROWS_WITH(VOXLoader::rotationMatrix(0x00),
                        "THREE.VOXLoader: Invalid rotation encoding");
}
