#include "loaders/MD2Loader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {
void pushInt32LE(std::vector<std::uint8_t>& out, std::int32_t value) {
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((raw >> (i * 8)) & 0xff));
}
void pushUint16LE(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(std::uint8_t(value & 0xff));
    out.push_back(std::uint8_t((value >> 8) & 0xff));
}
void pushFloat32LE(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 4; ++i) out.push_back(std::uint8_t((bits >> (i * 8)) & 0xff));
}

// Builds a header for the given counts and offsets, filling offset_end afterwards.
std::vector<std::uint8_t> header(std::int32_t numVertices, std::int32_t numSt,
                                 std::int32_t numTris, std::int32_t numFrames,
                                 std::int32_t offsetTris, std::int32_t offsetFrames) {
    std::vector<std::uint8_t> data;
    pushInt32LE(data, 844121161);
    pushInt32LE(data, 8);
    pushInt32LE(data, 64);
    pushInt32LE(data, 32);
    pushInt32LE(data, 0);
    pushInt32LE(data, 0);
    pushInt32LE(data, numVertices);
    pushInt32LE(data, numSt);
    pushInt32LE(data, numTris);
    pushInt32LE(data, 0);
    pushInt32LE(data, numFrames);
    pushInt32LE(data, 0);
    pushInt32LE(data, 68);
    pushInt32LE(data, offsetTris);
    pushInt32LE(data, offsetFrames);
    pushInt32LE(data, 0);
    pushInt32LE(data, 0);
    return data;
}
}  // namespace

TEST_CASE("MD2Loader reads triangle vertex and uv indices") {
    auto data = header(3, 3, 1, 0, 68, 0);
    // One triangle: vertices 0 1 2 then uvs 2 1 0 so a swap would show.
    pushUint16LE(data, 0);
    pushUint16LE(data, 1);
    pushUint16LE(data, 2);
    pushUint16LE(data, 2);
    pushUint16LE(data, 1);
    pushUint16LE(data, 0);
    const auto parsed = MD2Loader::parseHeader(data);
    REQUIRE(parsed.num_tris == 1);
    const auto triangles = MD2Loader::parseTriangles(data, parsed);
    REQUIRE(triangles.size() == 1);
    REQUIRE(triangles[0].vertex[0] == 0);
    REQUIRE(triangles[0].vertex[1] == 1);
    REQUIRE(triangles[0].vertex[2] == 2);
    REQUIRE(triangles[0].uv[0] == 2);
    REQUIRE(triangles[0].uv[1] == 1);
    REQUIRE(triangles[0].uv[2] == 0);
}

TEST_CASE("MD2Loader reads several triangles in order") {
    auto data = header(4, 4, 2, 0, 68, 0);
    for (int t = 0; t < 2; ++t) {
        for (int k = 0; k < 3; ++k) pushUint16LE(data, std::uint16_t(t * 3 + k));
        for (int k = 0; k < 3; ++k) pushUint16LE(data, std::uint16_t(k));
    }
    const auto parsed = MD2Loader::parseHeader(data);
    const auto triangles = MD2Loader::parseTriangles(data, parsed);
    REQUIRE(triangles.size() == 2);
    // The second triangle's first vertex index continues the numbering.
    REQUIRE(triangles[1].vertex[0] == 3);
    REQUIRE(triangles[1].vertex[2] == 5);
}

TEST_CASE("MD2Loader reads a frame scale, translation and name") {
    auto data = header(2, 0, 0, 1, 0, 68);
    pushFloat32LE(data, 2.0f);
    pushFloat32LE(data, 3.0f);
    pushFloat32LE(data, 4.0f);
    pushFloat32LE(data, 10.0f);
    pushFloat32LE(data, 20.0f);
    pushFloat32LE(data, 30.0f);
    for (const char c : std::string("run")) data.push_back(static_cast<std::uint8_t>(c));
    for (int i = 0; i < 13; ++i) data.push_back(0);  // pad the sixteen byte name
    // Two vertices at four bytes each, which the next frame would skip over.
    for (int i = 0; i < 8; ++i) data.push_back(0);

    const auto parsed = MD2Loader::parseHeader(data);
    const auto frames = MD2Loader::parseFrameHeaders(data, parsed);
    REQUIRE(frames.size() == 1);
    REQUIRE(frames[0].scale[0] == 2.0);
    REQUIRE(frames[0].scale[2] == 4.0);
    REQUIRE(frames[0].translation[0] == 10.0);
    REQUIRE(frames[0].translation[2] == 30.0);
    // The name stops at the first zero byte.
    REQUIRE(frames[0].name == "run");
}

TEST_CASE("MD2Loader walks past the vertex data to reach frame two") {
    auto data = header(1, 0, 0, 2, 0, 68);
    // Frame one.
    for (int i = 0; i < 6; ++i) pushFloat32LE(data, 1.0f);
    for (const char c : std::string("a")) data.push_back(static_cast<std::uint8_t>(c));
    for (int i = 0; i < 15; ++i) data.push_back(0);
    for (int i = 0; i < 4; ++i) data.push_back(0);  // one vertex
    // Frame two.
    for (int i = 0; i < 6; ++i) pushFloat32LE(data, 2.0f);
    for (const char c : std::string("b")) data.push_back(static_cast<std::uint8_t>(c));
    for (int i = 0; i < 15; ++i) data.push_back(0);
    for (int i = 0; i < 4; ++i) data.push_back(0);

    const auto parsed = MD2Loader::parseHeader(data);
    const auto frames = MD2Loader::parseFrameHeaders(data, parsed);
    REQUIRE(frames.size() == 2);
    REQUIRE(frames[0].name == "a");
    REQUIRE(frames[1].name == "b");
    REQUIRE(frames[1].scale[0] == 2.0);
}

TEST_CASE("MD2Loader fails when a frame name runs past the data") {
    auto data = header(0, 0, 0, 1, 0, 68);
    for (int i = 0; i < 6; ++i) pushFloat32LE(data, 1.0f);
    // No name bytes at all.
    const auto parsed = MD2Loader::parseHeader(data);
    REQUIRE_THROWS_WITH(MD2Loader::parseFrameHeaders(data, parsed),
                        "THREE.MD2Loader: Unexpected end of data");
}
