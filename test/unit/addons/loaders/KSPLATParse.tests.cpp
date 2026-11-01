#include "loaders/KSPLATLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
void putUint32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[at + i] = std::uint8_t((value >> (i * 8)) & 0xff);
}
void putUint16(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t value) {
    out[at] = std::uint8_t(value & 0xff);
    out[at + 1] = std::uint8_t((value >> 8) & 0xff);
}
void putFloat32(std::vector<std::uint8_t>& out, std::size_t at, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    putUint32(out, at, bits);
}

// A file with one section holding one uncompressed splat.
std::vector<std::uint8_t> minimalFile() {
    const std::size_t sectionData = 4096 + 1024;
    std::vector<std::uint8_t> out(sectionData + 44, 0);
    // The file header.
    out[0] = 0;
    out[1] = 1;
    putUint32(out, 4, 1);
    putUint32(out, 8, 1);
    putUint32(out, 12, 1);
    putUint32(out, 16, 1);
    putUint16(out, 20, 0);
    putFloat32(out, 36, -1.5f);
    putFloat32(out, 40, 1.5f);
    // The only section header, at four kilobytes.
    putUint32(out, 4096 + 0, 1);
    putUint32(out, 4096 + 4, 1);
    // One splat row at the section data.
    putFloat32(out, sectionData + 0, 1.0f);
    putFloat32(out, sectionData + 4, 2.0f);
    putFloat32(out, sectionData + 8, 3.0f);
    putFloat32(out, sectionData + 12, 0.5f);
    putFloat32(out, sectionData + 24, 1.0f);
    out[sectionData + 40] = 11;
    out[sectionData + 43] = 44;
    return out;
}
}  // namespace

TEST_CASE("KSPLATLoader parses a whole minimal file") {
    const auto buffer = minimalFile();
    const auto decoded = KSPLATLoader::parse(buffer);
    REQUIRE(decoded.count == 1);
    REQUIRE(decoded.degree == 0);
    // One centre of three components.
    REQUIRE(decoded.centers.size() == 3);
    REQUIRE(decoded.centers[0] == 1.0);
    REQUIRE(decoded.centers[2] == 3.0);
    // Six covariance components and four colour bytes.
    REQUIRE(decoded.covariances.size() == 6);
    REQUIRE(decoded.colors.size() == 4);
    REQUIRE(decoded.colors[0] == 11);
    REQUIRE(decoded.colors[3] == 44);
}

TEST_CASE("KSPLATLoader rejects a file whose section runs past the end") {
    auto buffer = minimalFile();
    // The section claims two splats but the file only holds one row.
    buffer.resize(4096 + 1024 + 44);
    putUint32(buffer, 4096 + 4, 2);
    REQUIRE_THROWS_WITH(KSPLATLoader::parse(buffer),
                        "THREE.KSPLATLoader: Invalid KSPLAT byte length.");
}

TEST_CASE("KSPLATLoader rejects a truncated file") {
    const std::vector<std::uint8_t> buffer(100, 0);
    REQUIRE_THROWS_WITH(KSPLATLoader::parse(buffer),
                        "THREE.KSPLATLoader: Invalid KSPLAT header.");
}