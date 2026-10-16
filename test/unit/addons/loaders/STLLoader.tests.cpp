#include "catch.hpp"
#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "loaders/STLLoader.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// The binary path is exercised through parseBytes, because a binary STL carries NUL bytes that a
// JS string cannot round trip, and the resulting geometry is read back through the JS surface.
namespace {
class Writer {
  public:
    void u32(std::uint32_t value) { for (int shift = 0; shift < 32; shift += 8) bytes.push_back(std::uint8_t((value >> shift) & 0xff)); }
    void u16(std::uint16_t value) { bytes.push_back(std::uint8_t(value & 0xff)); bytes.push_back(std::uint8_t((value >> 8) & 0xff)); }
    void f32(float value) { std::uint32_t bits = 0; std::memcpy(&bits, &value, sizeof(bits)); u32(bits); }
    void raw(const std::string& text) { for (char ch : text) bytes.push_back(std::uint8_t(ch)); }
    void zeros(std::size_t count) { for (std::size_t index = 0; index < count; index += 1) bytes.push_back(0); }
    std::vector<std::uint8_t> take() { return bytes; }
  private:
    std::vector<std::uint8_t> bytes;
};
// Each triangle is (0,0,0) (1,0,0) (0,1,0) with the normal (0,0,1), followed by a packed colour.
std::vector<std::uint8_t> build(std::size_t faces, const std::string& header, const std::vector<std::uint16_t>& packed) {
    Writer writer;
    writer.raw(header);
    writer.zeros(80 - header.size());
    writer.u32(std::uint32_t(faces));
    for (std::size_t face = 0; face < faces; face += 1) {
        writer.f32(0.f); writer.f32(0.f); writer.f32(1.f);
        writer.f32(0.f); writer.f32(0.f); writer.f32(0.f);
        writer.f32(1.f); writer.f32(0.f); writer.f32(0.f);
        writer.f32(0.f); writer.f32(1.f); writer.f32(0.f);
        writer.u16(face < packed.size() ? packed[face] : 0);
    }
    return writer.take();
}
double countOf(const JSValue& geometry, const char* attribute) {
    return geometry.get("attributes").get(attribute).get("count").toNumber();
}
double arrayAt(const JSValue& geometry, const char* attribute, double index) {
    return geometry.get("attributes").get(attribute).get("array").get(index).toNumber();
}
bool hasAttribute(const JSValue& geometry, const char* attribute) {
    const auto value = geometry.get("attributes").get(attribute);
    return !value.isUndefined() && !value.isNull();
}
JSValue geometryOf(const std::vector<std::uint8_t>& bytes) {
    STLLoader loader;
    return nativeGeometry(loader.parseBytes(bytes));
}
}

TEST_CASE("STLLoader reads a binary triangle", "[stlloader]") {
    const auto geometry = geometryOf(build(1, "solid test", {}));
    REQUIRE(countOf(geometry, "position") == 3);
    REQUIRE(countOf(geometry, "normal") == 3);
    REQUIRE(arrayAt(geometry, "position", 0) == 0.0);
    REQUIRE(arrayAt(geometry, "position", 3) == 1.0);
    REQUIRE(arrayAt(geometry, "position", 7) == 1.0);
    REQUIRE(arrayAt(geometry, "normal", 2) == 1.0);
    REQUIRE_FALSE(hasAttribute(geometry, "color"));
    REQUIRE(geometry.get("hasColors").isUndefined());
}

TEST_CASE("STLLoader decides binary from the predicted length and the solid prefix", "[stlloader]") {
    REQUIRE(STLLoader::isBinary(build(2, "header", {})));
    // A length that does not match the predicted face count falls through to the prefix test, and
    // a header without the word solid leaves it binary.
    auto binary = build(2, "header", {});
    binary.pop_back();
    REQUIRE(STLLoader::isBinary(binary));
    // The same truncation with a solid prefix is read as ASCII instead.
    auto ascii = build(2, "solid ", {});
    ascii.pop_back();
    REQUIRE_FALSE(STLLoader::isBinary(ascii));
    const std::vector<std::uint8_t> plain{'s', 'o', 'l', 'i', 'd', ' ', 'x'};
    REQUIRE_FALSE(STLLoader::isBinary(plain));
    // Up to five bytes of an encoding prefix may precede "solid".
    const std::vector<std::uint8_t> prefixed{0xef, 0xbb, 0xbf, 's', 'o', 'l', 'i', 'd'};
    REQUIRE_FALSE(STLLoader::isBinary(prefixed));
}

TEST_CASE("STLLoader fails loudly on an ASCII document", "[stlloader]") {
    STLLoader loader;
    const std::string ascii = "solid test\nfacet normal 0 0 1\nendfacet\nendsolid test\n";
    bool threw = false;
    try {
        loader.parseBytes(std::vector<std::uint8_t>(ascii.begin(), ascii.end()));
    } catch (const JSTypeError& error) {
        threw = true;
        REQUIRE(std::string(error.what()).find("ASCII parsing is not implemented") != std::string::npos);
    }
    REQUIRE(threw);
}

TEST_CASE("STLLoader reads the Magics header colour and packed face colours", "[stlloader]") {
    std::string header = "solid ";
    header += "COLOR=";
    header.push_back(char(200)); header.push_back(char(100)); header.push_back(char(50)); header.push_back(char(255));
    const auto geometry = geometryOf(build(1, header, {0x001f}));
    REQUIRE(hasAttribute(geometry, "color"));
    REQUIRE(geometry.get("hasColors").toBoolean());
    REQUIRE(geometry.get("alpha").toNumber() == 1.0);
    REQUIRE(countOf(geometry, "color") == 3);
    // The packed colour is five bits per channel over 31, then converted from sRGB. 0x001f is a
    // full red channel, so red is one and the other two are zero.
    REQUIRE(arrayAt(geometry, "color", 0) == 1.0);
    REQUIRE(arrayAt(geometry, "color", 1) == 0.0);
    REQUIRE(arrayAt(geometry, "color", 2) == 0.0);
    // Three vertices per face, so the second vertex starts at index 3 and carries the same colour.
    REQUIRE(arrayAt(geometry, "color", 3) == 1.0);
}