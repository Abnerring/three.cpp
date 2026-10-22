#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void putFloat32(std::vector<std::uint8_t>& out, std::size_t at, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((bits >> (i * 8)) & 0xff);
}
// A level zero row: centre, scale, rotation, colour, forty four bytes.
std::vector<std::uint8_t> level0Row() {
    std::vector<std::uint8_t> out(44, 0);
    putFloat32(out, 0, 1.0f);
    putFloat32(out, 4, 2.0f);
    putFloat32(out, 8, 3.0f);
    putFloat32(out, 12, 0.5f);
    putFloat32(out, 16, 0.25f);
    putFloat32(out, 20, 0.125f);
    putFloat32(out, 24, 1.0f);
    putFloat32(out, 28, 0.0f);
    putFloat32(out, 32, 0.0f);
    putFloat32(out, 36, 0.0f);
    out[40] = 10;
    out[41] = 20;
    out[42] = 30;
    out[43] = 40;
    return out;
}
}  // namespace

TEST_CASE("KSPLATLoader decodes an uncompressed row") {
    const auto buffer = level0Row();
    KSPLATLoader::SectionHeader section;
    double bucket[3] = {0, 0, 0};
    const auto row = KSPLATLoader::decodeRow(buffer, 0, section, KSPLATLoader::compression(0), bucket);
    // The centre is the three floats as written.
    REQUIRE(row.center[0] == 1.0);
    REQUIRE(row.center[1] == 2.0);
    REQUIRE(row.center[2] == 3.0);
    // The scale is stored directly, not as a logarithm.
    REQUIRE(row.scale[0] == 0.5);
    REQUIRE(row.scale[2] == 0.125);
    // The rotation is w x y z.
    REQUIRE(row.rotation[0] == 1.0);
    REQUIRE(row.rotation[1] == 0.0);
    // The colour is the four trailing bytes.
    REQUIRE(row.color[0] == 10);
    REQUIRE(row.color[3] == 40);
}

TEST_CASE("KSPLATLoader scales with half floats in a compressed row") {
    // A compressed row is six, six, eight and four bytes: six plus six puts the scale at six
    // and the rotation at twelve, and the colour at twenty.
    std::vector<std::uint8_t> buffer(24, 0);
    // Centre shorts at the middle of the range place the splat on its bucket.
    buffer[0] = 0xff;
    buffer[1] = 0x7f;
    buffer[2] = 0xff;
    buffer[3] = 0x7f;
    buffer[4] = 0xff;
    buffer[5] = 0x7f;
    // The first scale is a half float of one.
    buffer[6] = 0x00;
    buffer[7] = 0x3c;
    // The first rotation component is a half float of one as well.
    buffer[12] = 0x00;
    buffer[13] = 0x3c;
    buffer[20] = 7;

    KSPLATLoader::SectionHeader section;
    section.bucketBlockSize = 4.0;
    section.compressionScaleRange = 32767;
    double bucket[3] = {10.0, 20.0, 30.0};
    const auto row = KSPLATLoader::decodeRow(buffer, 0, section, KSPLATLoader::compression(1), bucket);
    // The middle stored value lands on each bucket coordinate.
    REQUIRE(row.center[0] == Approx(10.0).epsilon(1e-9));
    REQUIRE(row.center[2] == Approx(30.0).epsilon(1e-9));
    REQUIRE(row.scale[0] == 1.0);
    REQUIRE(row.rotation[0] == 1.0);
    REQUIRE(row.color[0] == 7);
}

TEST_CASE("KSPLATLoader leaves a short row alone") {
    const std::vector<std::uint8_t> buffer(10, 0);
    KSPLATLoader::SectionHeader section;
    double bucket[3] = {0, 0, 0};
    const auto row = KSPLATLoader::decodeRow(buffer, 0, section, KSPLATLoader::compression(0), bucket);
    // A row that does not fit returns the defaults rather than reading past the end.
    REQUIRE(row.center[0] == 0.0);
    REQUIRE(row.scale[0] == 1.0);
    REQUIRE(row.rotation[0] == 1.0);
}