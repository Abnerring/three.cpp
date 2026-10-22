#include "loaders/PLYLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("PLYLoader reports the binary widths of each type") {
    REQUIRE(PLYLoader::binaryTypeSize("char") == 1);
    REQUIRE(PLYLoader::binaryTypeSize("uchar") == 1);
    REQUIRE(PLYLoader::binaryTypeSize("short") == 2);
    REQUIRE(PLYLoader::binaryTypeSize("ushort") == 2);
    REQUIRE(PLYLoader::binaryTypeSize("int") == 4);
    REQUIRE(PLYLoader::binaryTypeSize("uint") == 4);
    REQUIRE(PLYLoader::binaryTypeSize("float") == 4);
    REQUIRE(PLYLoader::binaryTypeSize("double") == 8);
    REQUIRE(PLYLoader::binaryTypeSize("nonsense") == 0);
}

TEST_CASE("PLYLoader reads both endiannesses") {
    const std::vector<PLYLoader::Property> properties{{"ushort", "v", "", ""}};
    // 0x1234 stored little endian is 34 12, big endian is 12 34.
    const std::vector<std::uint8_t> little{0x34, 0x12};
    std::size_t at = 0;
    const auto littleRead = PLYLoader::binaryReadElement(little, at, properties, true);
    REQUIRE(littleRead.has_value());
    REQUIRE(std::get<double>(littleRead->first.at("v")) == 0x1234);

    const std::vector<std::uint8_t> big{0x12, 0x34};
    const auto bigRead = PLYLoader::binaryReadElement(big, at, properties, false);
    REQUIRE(bigRead.has_value());
    REQUIRE(std::get<double>(bigRead->first.at("v")) == 0x1234);
}

TEST_CASE("PLYLoader reads signed values and floats") {
    // int16 -2 little endian is fe ff.
    const std::vector<PLYLoader::Property> signedProp{{"short", "v", "", ""}};
    const std::vector<std::uint8_t> signedBytes{0xfe, 0xff};
    std::size_t at = 0;
    const auto signedRead = PLYLoader::binaryReadElement(signedBytes, at, signedProp, true);
    REQUIRE(signedRead.has_value());
    REQUIRE(std::get<double>(signedRead->first.at("v")) == -2.0);

    // float32 one point five little endian is 00 00 c0 3f.
    const std::vector<PLYLoader::Property> floatProp{{"float", "v", "", ""}};
    const std::vector<std::uint8_t> floatBytes{0x00, 0x00, 0xc0, 0x3f};
    const auto floatRead = PLYLoader::binaryReadElement(floatBytes, at, floatProp, true);
    REQUIRE(floatRead.has_value());
    REQUIRE(std::get<double>(floatRead->first.at("v")) == Approx(1.5).epsilon(1e-6));
}

TEST_CASE("PLYLoader reads a binary list and reports the bytes consumed") {
    const std::vector<PLYLoader::Property> properties{{"list", "vertex_indices", "uchar", "int"}};
    // Count of two, then the two indices 5 and 6 as little endian int32.
    const std::vector<std::uint8_t> data{0x02, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
    std::size_t at = 0;
    const auto read = PLYLoader::binaryReadElement(data, at, properties, true);
    REQUIRE(read.has_value());
    REQUIRE(std::get<std::vector<double>>(read->first.at("vertex_indices")).size() == 2);
    REQUIRE(std::get<std::vector<double>>(read->first.at("vertex_indices"))[1] == 6.0);
    // One count byte plus two four byte indices.
    REQUIRE(read->second == 9);
}

TEST_CASE("PLYLoader fails when the binary data ends early") {
    const std::vector<PLYLoader::Property> properties{{"float", "x", "", ""}, {"float", "y", "", ""}};
    const std::vector<std::uint8_t> shortData{0x00, 0x00, 0xc0, 0x3f};
    std::size_t at = 0;
    REQUIRE_FALSE(PLYLoader::binaryReadElement(shortData, at, properties, true).has_value());
}
