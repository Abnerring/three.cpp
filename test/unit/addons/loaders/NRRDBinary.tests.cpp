#include "loaders/NRRDLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

TEST_CASE("NRRDLoader reports the width of each type") {
    REQUIRE(NRRDLoader::typeWidth("uchar") == 1);
    REQUIRE(NRRDLoader::typeWidth("schar") == 1);
    REQUIRE(NRRDLoader::typeWidth("ushort") == 2);
    REQUIRE(NRRDLoader::typeWidth("sshort") == 2);
    REQUIRE(NRRDLoader::typeWidth("uint") == 4);
    REQUIRE(NRRDLoader::typeWidth("sint") == 4);
    REQUIRE(NRRDLoader::typeWidth("float") == 4);
    REQUIRE(NRRDLoader::typeWidth("double") == 8);
    // An unknown name has no width.
    REQUIRE(NRRDLoader::typeWidth("nonsense") == 0);
}

TEST_CASE("NRRDLoader reads unsigned and signed bytes") {
    const std::vector<std::uint8_t> data{1, 2, 255};
    const auto unsignedValues = NRRDLoader::parseBinaryData(data, 0, 3, "uchar");
    REQUIRE(unsignedValues.size() == 3);
    REQUIRE(unsignedValues[2] == 255.0);

    // The same byte read as signed is minus one.
    const std::vector<std::uint8_t> one{255};
    const auto signedValue = NRRDLoader::parseBinaryData(one, 0, 1, "schar");
    REQUIRE(signedValue[0] == -1.0);
}

TEST_CASE("NRRDLoader reads little endian shorts") {
    // 0x1234 stored little endian.
    const std::vector<std::uint8_t> data{0x34, 0x12};
    const auto values = NRRDLoader::parseBinaryData(data, 0, 1, "ushort");
    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 0x1234);
}

TEST_CASE("NRRDLoader reads signed shorts and ints") {
    // Minus one as a two byte value, then as a four byte value.
    const std::vector<std::uint8_t> shorts{0xff, 0xff};
    REQUIRE(NRRDLoader::parseBinaryData(shorts, 0, 1, "sshort")[0] == -1.0);
    const std::vector<std::uint8_t> ints{0xff, 0xff, 0xff, 0xff};
    REQUIRE(NRRDLoader::parseBinaryData(ints, 0, 1, "sint")[0] == -1.0);
}

TEST_CASE("NRRDLoader reads little endian floats") {
    // 1.0 as a float is 0x3f800000, stored little endian.
    std::vector<std::uint8_t> data;
    const float value = 1.0f;
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 4; ++i) data.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
    const auto values = NRRDLoader::parseBinaryData(data, 0, 1, "float");
    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 1.0);
}

TEST_CASE("NRRDLoader reads doubles") {
    std::vector<std::uint8_t> data;
    const double value = 2.5;
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 8; ++i) data.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
    const auto values = NRRDLoader::parseBinaryData(data, 0, 1, "double");
    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 2.5);
}

TEST_CASE("NRRDLoader stops when the body runs out") {
    const std::vector<std::uint8_t> data{1, 2};
    // Three shorts were asked for but only one fits.
    const auto values = NRRDLoader::parseBinaryData(data, 0, 3, "ushort");
    REQUIRE(values.size() == 1);
}

TEST_CASE("NRRDLoader rejects an unknown type") {
    const std::vector<std::uint8_t> data{1};
    REQUIRE_THROWS_WITH(NRRDLoader::parseBinaryData(data, 0, 1, "nonsense"),
                        "THREE.NRRDLoader: Unknown type nonsense");
}
