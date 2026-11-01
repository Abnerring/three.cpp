#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>

TEST_CASE("EXRLoader tests the leftover column by bit not by remainder") {
    // A step of one leaves a leftover on every odd row.
    REQUIRE_FALSE(EXRLoader::pizHasOddColumn(2, 1));
    REQUIRE(EXRLoader::pizHasOddColumn(3, 1));
    // A step of four asks whether the row has the fours bit: five does and four does not.
    // Four has the fours bit itself, so it does have a leftover.
    REQUIRE(EXRLoader::pizHasOddColumn(4, 4));
    REQUIRE(EXRLoader::pizHasOddColumn(5, 4));
    REQUIRE(EXRLoader::pizHasOddColumn(6, 4));
    REQUIRE_FALSE(EXRLoader::pizHasOddColumn(8, 4));
    // A remainder test would have said a row of six was even and left it without a leftover,
    // which is why this distinction is worth pinning down.
    REQUIRE(EXRLoader::pizHasOddColumn(6, 4));
    REQUIRE(6 % 4 != 0);
    REQUIRE(EXRLoader::pizHasOddColumn(7, 4));
    REQUIRE_FALSE(EXRLoader::pizHasOddColumn(7, 8));
}

TEST_CASE("EXRLoader walks the four places one step touches") {
    // Starting at ten with a horizontal stride of two and a vertical one of forty.
    const auto offsets = EXRLoader::pizOffsets(10, 2, 40);
    REQUIRE(offsets.p00 == 10);
    REQUIRE(offsets.p01 == 12);
    REQUIRE(offsets.p10 == 50);
    // The fourth is one of each away, which is the third plus the first stride.
    REQUIRE(offsets.p11 == 52);
    REQUIRE(offsets.p11 == offsets.p10 + 2);
    // With unit strides the four are consecutive corners of a square.
    const auto unit = EXRLoader::pizOffsets(0, 1, 1);
    REQUIRE(unit.p00 == 0);
    REQUIRE(unit.p01 == 1);
    REQUIRE(unit.p10 == 1);
    REQUIRE(unit.p11 == 2);
}

TEST_CASE("EXRLoader writes the four places in the order it read them") {
    const auto& order = EXRLoader::pizWriteOrder();
    REQUIRE(order.size() == 4);
    REQUIRE(order[0] == 0);
    REQUIRE(order[1] == 1);
    REQUIRE(order[2] == 2);
    REQUIRE(order[3] == 3);
    // Ascending, so no place is written before it has been read.
    for (std::size_t i = 1; i < order.size(); ++i) {
        REQUIRE(order[i] > order[i - 1]);
    }
}
