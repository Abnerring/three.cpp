#include "loaders/TIFFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
void pushUint16(std::vector<std::uint8_t>& out, std::uint16_t value, bool little) {
    if (little) {
        out.push_back(std::uint8_t(value & 0xff));
        out.push_back(std::uint8_t((value >> 8) & 0xff));
    } else {
        out.push_back(std::uint8_t((value >> 8) & 0xff));
        out.push_back(std::uint8_t(value & 0xff));
    }
}
void pushUint32(std::vector<std::uint8_t>& out, std::uint32_t value, bool little) {
    for (int i = 0; i < 4; ++i) {
        const int shift = little ? i * 8 : (3 - i) * 8;
        out.push_back(std::uint8_t((value >> shift) & 0xff));
    }
}
// Builds a little endian file with two directory entries: width and height.
std::vector<std::uint8_t> simpleFile(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> out{'I', 'I'};
    pushUint16(out, 42, true);
    pushUint32(out, 8, true);
    pushUint16(out, 2, true);
    // Width, type long, one value.
    pushUint16(out, 256, true);
    pushUint16(out, 4, true);
    pushUint32(out, 1, true);
    pushUint32(out, width, true);
    // Height.
    pushUint16(out, 257, true);
    pushUint16(out, 4, true);
    pushUint32(out, 1, true);
    pushUint32(out, height, true);
    // A zero offset ends the directory list.
    pushUint32(out, 0, true);
    return out;
}
}  // namespace

TEST_CASE("TIFFLoader reads a little endian header") {
    const auto header = TIFFLoader::readHeader(simpleFile(640, 480));
    REQUIRE(header.littleEndian);
    REQUIRE(header.width == 640);
    REQUIRE(header.height == 480);
    // Two entries were listed.
    REQUIRE(header.directoryCount == 2);
    // Uncompressed is the default.
    REQUIRE(header.compression == 1);
}

TEST_CASE("TIFFLoader reads a big endian header") {
    std::vector<std::uint8_t> out{'M', 'M'};
    pushUint16(out, 42, false);
    pushUint32(out, 8, false);
    pushUint16(out, 2, false);
    pushUint16(out, 256, false);
    pushUint16(out, 4, false);
    pushUint32(out, 1, false);
    pushUint32(out, 300, false);
    pushUint16(out, 257, false);
    pushUint16(out, 4, false);
    pushUint32(out, 1, false);
    pushUint32(out, 200, false);
    pushUint32(out, 0, false);

    const auto header = TIFFLoader::readHeader(out);
    REQUIRE_FALSE(header.littleEndian);
    REQUIRE(header.width == 300);
    REQUIRE(header.height == 200);
}

TEST_CASE("TIFFLoader reads a short valued tag") {
    // A bits per sample entry uses the short type, two bytes wide.
    std::vector<std::uint8_t> out = simpleFile(2, 2);
    // Append one more entry by rewriting the count and the trailing offset.
    const std::size_t countAt = 8;
    out[countAt] = 3;
    const std::size_t entryAt = 8 + 2 + 2 * 12;
    std::vector<std::uint8_t> extra;
    pushUint16(extra, 258, true);
    pushUint16(extra, 3, true);
    pushUint32(extra, 1, true);
    pushUint16(extra, 8, true);
    pushUint16(extra, 0, true);
    out.insert(out.begin() + static_cast<std::ptrdiff_t>(entryAt), extra.begin(), extra.end());
    const auto header = TIFFLoader::readHeader(out);
    REQUIRE(header.bitsPerSample == 8);
}

TEST_CASE("TIFFLoader rejects a file that is not TIFF") {
    const std::vector<std::uint8_t> wrong{'P', 'K', 3, 4, 5, 6, 7, 8};
    REQUIRE_THROWS_WITH(TIFFLoader::readHeader(wrong), "THREE.TIFFLoader: Not a TIFF file");
    // A correct mark with the wrong magic is rejected too.
    std::vector<std::uint8_t> badMagic{'I', 'I', 43, 0, 8, 0, 0, 0};
    REQUIRE_THROWS_WITH(TIFFLoader::readHeader(badMagic), "THREE.TIFFLoader: Not a TIFF file");
    // So is a buffer too short to hold the header.
    REQUIRE_THROWS_WITH(TIFFLoader::readHeader({'I'}), "THREE.TIFFLoader: Not a TIFF file");
}

TEST_CASE("TIFFLoader reports a directory beyond the file") {
    std::vector<std::uint8_t> out{'I', 'I'};
    pushUint16(out, 42, true);
    pushUint32(out, 1000, true);
    REQUIRE_THROWS_WITH(TIFFLoader::readHeader(out), "THREE.TIFFLoader: Directory runs past the file");
}

TEST_CASE("TIFFLoader refuses to decode pixels") {
    REQUIRE_THROWS_WITH(TIFFLoader::requireDecoder(),
                        "THREE.TIFFLoader: Decoding needs the UTIF library, which is not ported");
}