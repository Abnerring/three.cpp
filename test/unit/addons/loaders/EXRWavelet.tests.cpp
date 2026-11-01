#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader masks and sign extends sixteen bits") {
    // The mask keeps the low sixteen bits.
    REQUIRE(EXRLoader::uint16Bits(0) == 0);
    REQUIRE(EXRLoader::uint16Bits(0xFFFF) == 0xFFFF);
    REQUIRE(EXRLoader::uint16Bits(-1) == 0xFFFF);
    REQUIRE(EXRLoader::uint16Bits(0x12345) == 0x2345);
    // The signed reading pulls anything above the maximum down a whole range.
    REQUIRE(EXRLoader::int16Bits(0x7FFF) == 32767);
    REQUIRE(EXRLoader::int16Bits(0x8000) == -32768);
    REQUIRE(EXRLoader::int16Bits(0xFFFF) == -1);
    REQUIRE(EXRLoader::int16Bits(-1) == -1);
}

TEST_CASE("EXRLoader decodes a wavelet step") {
    // A zero high coefficient leaves the two samples equal to the low one.
    const auto flat = EXRLoader::wdec14(0, 0);
    REQUIRE(flat.a == 0);
    REQUIRE(flat.b == 0);
    const auto same = EXRLoader::wdec14(10, 0);
    REQUIRE(same.a == 10);
    REQUIRE(same.b == 10);
    // An even high coefficient adds half of it to the low one, and the pair differs by it.
    const auto even = EXRLoader::wdec14(10, 4);
    REQUIRE(even.a == 12);
    REQUIRE(even.b == 8);
    // An odd one also adds the parity, so the two stay whole.
    const auto odd = EXRLoader::wdec14(10, 5);
    REQUIRE(odd.a == 13);
    REQUIRE(odd.b == 8);
}

TEST_CASE("EXRLoader turns luminance and colour differences back") {
    // Pure luminance is grey: the three channels all come out the same.
    auto grey = EXRLoader::YcbcrPixel{1.0, 0.0, 0.0};
    EXRLoader::csc709Inverse(grey);
    REQUIRE(grey.y == 1.0);
    REQUIRE(grey.cb == 1.0);
    REQUIRE(grey.cr == 1.0);

    // A pure blue difference lifts the third channel by its own coefficient.
    auto blue = EXRLoader::YcbcrPixel{0.0, 1.0, 0.0};
    EXRLoader::csc709Inverse(blue);
    REQUIRE(blue.y == 0.0);
    REQUIRE(blue.cb == Approx(-0.1873).epsilon(1e-12));
    REQUIRE(blue.cr == Approx(1.8556).epsilon(1e-12));

    // A pure red difference lifts the first channel and pulls the second.
    auto red = EXRLoader::YcbcrPixel{0.0, 0.0, 1.0};
    EXRLoader::csc709Inverse(red);
    REQUIRE(red.y == Approx(1.5747).epsilon(1e-12));
    REQUIRE(red.cb == Approx(-0.4682).epsilon(1e-12));
    REQUIRE(red.cr == 0.0);
}

TEST_CASE("EXRLoader transforms every pixel of a block") {
    // Two pixels, one grey and one blue, both transformed in place.
    EXRLoader::YcbcrPixel block[2] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    EXRLoader::csc709InverseBlock(block, 2);
    REQUIRE(block[0].y == 1.0);
    REQUIRE(block[1].cr == Approx(1.8556).epsilon(1e-12));
    // A count of zero touches nothing.
    EXRLoader::YcbcrPixel one{5.0, 0.0, 0.0};
    EXRLoader::csc709InverseBlock(&one, 0);
    REQUIRE(one.y == 5.0);
}