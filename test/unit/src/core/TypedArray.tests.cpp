#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"

TEST_CASE("TypedArray aliases and subarrays share bytes without sharing view identity", "[typedarray]") {
    Float32Array values({1, 2, 3, 4});
    auto alias = values;
    auto view = values.subarray(1, 3);
    REQUIRE(alias.sameIdentity(values));
    REQUIRE_FALSE(view.sameIdentity(values));
    REQUIRE(view.buffer.sameIdentity(values.buffer));
    REQUIRE(std::size_t(view.byteOffset) == 4);
    REQUIRE(std::size_t(view.byteLength) == 8);
    REQUIRE(std::size_t(view.length) == 2);
    view[0] = 9;
    REQUIRE(values[1] == 9);
    auto copy = values.slice();
    REQUIRE_FALSE(copy.buffer.sameIdentity(values.buffer));
    copy[1] = 7;
    REQUIRE(values[1] == 9);
    REQUIRE_FALSE(values.get(4).type() == JSValue::Type::Number);
    REQUIRE(values.get(-1).isUndefined());
    REQUIRE(values.get(.5).isUndefined());
    values.setIndex(4, 99);
    REQUIRE(values[3] == 4);
}

TEST_CASE("ArrayBuffer typed views reinterpret bytes and enforce view bounds", "[typedarray]") {
    ArrayBuffer storage(8);
    Uint32Array words(storage);
    Float32Array floats(storage);
    words[0] = 0x3f800000;
    REQUIRE(floats[0] == 1);
    REQUIRE(words.buffer.sameIdentity(floats.buffer));
    REQUIRE_THROWS_AS(Float32Array(storage, 1), JSRangeError);
    REQUIRE_THROWS_AS(Float32Array(storage, 4, 2), JSRangeError);
    REQUIRE_THROWS_AS(Float32Array(ArrayBuffer(3)), JSRangeError);
    auto bytes = storage.slice(0, 4);
    REQUIRE_FALSE(bytes.sameIdentity(storage));
    REQUIRE(Uint32Array(bytes)[0] == 0x3f800000);
    REQUIRE_THROWS_AS(ArrayBuffer(-1), JSRangeError);
    REQUIRE_THROWS_AS(Float32Array(-1), JSRangeError);
    REQUIRE_THROWS_AS(Float32Array(std::numeric_limits<double>::infinity()), JSRangeError);
    REQUIRE(Float32Array(std::numeric_limits<double>::quiet_NaN()).size() == 0);
    REQUIRE(Float32Array(2.9).size() == 2);
    REQUIRE(Uint8Array(storage, 1.9, 2.9).size() == 2);
    REQUIRE(std::size_t(Uint8Array(storage, 1.9, 2.9).byteOffset) == 1);
}

TEST_CASE("TypedArray iterators retain original views across handle rebinding and destruction", "[typedarray]") {
    Float32Array array({1, 2, 3});
    auto original = array;
    auto iterator = array.begin();
    auto reference = array[1];
    array = Float32Array({9, 8});
    REQUIRE(float(*iterator) == 1);
    *iterator = 4;
    reference = 5;
    REQUIRE(original[0] == 4);
    REQUIRE(original[1] == 5);
    auto borrowed = [&] { auto alias = original; return alias.begin(); }();
    REQUIRE(float(*borrowed) == 4);
    std::reverse(original.begin(), original.end());
    REQUIRE(original[0] == 3);
    REQUIRE(original[2] == 4);
    std::ranges::fill(original.begin(), original.end(), 7);
    REQUIRE(original[0] == 7);
    REQUIRE(original[2] == 7);
}

TEST_CASE("TypedArray set snapshots overlapping views and converts numeric storage", "[typedarray]") {
    Uint8Array bytes({1, 2, 3, 4});
    bytes.set(bytes.subarray(0, 3), 1);
    REQUIRE(bytes[0] == 1);
    REQUIRE(bytes[1] == 1);
    REQUIRE(bytes[2] == 2);
    REQUIRE(bytes[3] == 3);
    Int8Array signedBytes(4);
    signedBytes.set(std::vector<double>{257, -129, std::numeric_limits<double>::infinity(), 3.9});
    REQUIRE(signedBytes[0] == 1);
    REQUIRE(signedBytes[1] == 127);
    REQUIRE(signedBytes[2] == 0);
    REQUIRE(signedBytes[3] == 3);
    REQUIRE_THROWS_AS(bytes.set(std::vector<double>{9}, -1), JSRangeError);
    REQUIRE_THROWS_AS(bytes.set(std::vector<double>{9, 9}, 3), JSRangeError);
    bytes.set(std::vector<double>{7}, std::numeric_limits<double>::quiet_NaN());
    REQUIRE(bytes[0] == 7);
    Uint32Array nanBits({0x7f800001u, 0x80000000u});
    Float32Array nanSource(nanBits.buffer);
    Float32Array nanCopy(2);
    nanCopy.set(nanSource);
    Uint32Array copiedBits(nanCopy.buffer);
    REQUIRE(copiedBits[0] == 0x7f800001u);
    REQUIRE(copiedBits[1] == 0x80000000u);
}

TEST_CASE("Uint8ClampedArray saturates and rounds ties to even", "[typedarray]") {
    Uint8ClampedArray values(8);
    values.set(std::vector<double>{-.5, .5, 1.5, 2.5, 254.5, 255.5,
                                   std::numeric_limits<double>::quiet_NaN(), 300});
    const std::vector<int> expected{0, 0, 2, 2, 254, 255, 0, 255};
    for (std::size_t i = 0; i < values.size(); ++i) REQUIRE(values[i] == expected[i]);
    BufferAttribute attribute(values, 1, true);
    REQUIRE(attribute.arrayType() == "Uint8ClampedArray");
    attribute.setX(0, 2);
    REQUIRE(values[0] == 255);
    REQUIRE(attribute.getX(0) == 1);
}

TEST_CASE("BufferAttribute preserves incoming view identity while typed subclasses and copies clone", "[attribute][typedarray]") {
    Float32Array values({1, 2, 3, 4});
    auto view = values.subarray(1, 3);
    BufferAttribute direct(view, 1);
    REQUIRE(std::get<Float32Array>(direct.array).sameIdentity(view));
    view[0] = 8;
    REQUIRE(direct.getX(0) == 8);
    Float32BufferAttribute typed(view, 1);
    REQUIRE_FALSE(std::get<Float32Array>(typed.array).sameIdentity(view));
    REQUIRE_FALSE(std::get<Float32Array>(typed.array).buffer.sameIdentity(view.buffer));
    auto clone = direct.clone();
    REQUIRE_FALSE(std::get<Float32Array>(clone->array).buffer.sameIdentity(view.buffer));
    direct.setX(0, 9);
    REQUIRE(typed.getX(0) == 8);
    REQUIRE(clone->getX(0) == 8);
    direct.copy(direct);
    REQUIRE_FALSE(std::get<Float32Array>(direct.array).sameIdentity(view));
    REQUIRE(direct.getX(0) == 9);
    Float32BufferAttribute converted(Uint8Array({1, 2, 3}), 3);
    REQUIRE(converted.getZ(0) == 3);
    Uint8ClampedBufferAttribute convertedClamp(std::vector<double>{.5, 1.5, 300}, 1);
    REQUIRE(convertedClamp.getX(0) == 0);
    REQUIRE(convertedClamp.getX(1) == 2);
    REQUIRE(convertedClamp.getX(2) == 255);
}

TEST_CASE("toNonIndexed preserves clamped storage constructor", "[attribute][typedarray]") {
    BufferGeometry geometry;
    geometry.setAttribute("position", std::make_shared<Uint8ClampedBufferAttribute>(
        std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6}, 3));
    geometry.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{1, 0}, 1));
    auto attribute = std::dynamic_pointer_cast<BufferAttribute>(geometry.toNonIndexed()->getAttribute("position"));
    REQUIRE(attribute->arrayType() == "Uint8ClampedArray");
    REQUIRE(attribute->getX(0) == 4);
    geometry.index->count = 1;
    REQUIRE(geometry.toNonIndexed()->getAttribute("position")->getCount() == 2);
    geometry.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{2}, 1));
    REQUIRE(geometry.toNonIndexed()->getAttribute("position")->getX(0) == 0);
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3}, 3));
    REQUIRE(std::isnan(geometry.toNonIndexed()->getAttribute("position")->getX(0)));
}

TEST_CASE("BufferAttribute preserves fractional counts and numeric index arithmetic", "[attribute][typedarray]") {
    Float32BufferAttribute attribute({1, 2, 3, 4}, 3);
    REQUIRE(attribute.count == 4.0 / 3.0);
    REQUIRE(attribute.getX(1.0 / 3.0) == 2);
    REQUIRE(std::isnan(attribute.getX(.5)));
    REQUIRE(attribute.getComponent(-1, 3) == 1);
    attribute.setComponent(-1, 4, 8);
    REQUIRE(attribute.getX(1.0 / 3.0) == 8);
    attribute.applyMatrix4(Matrix4());
    REQUIRE(std::isnan(attribute.getX(1)));
    Float32BufferAttribute zero({1, 2}, 0);
    REQUIRE(std::isinf(zero.count));
    REQUIRE(zero.getX(99) == 1);
    Float32BufferAttribute empty(std::vector<float>{}, 0);
    REQUIRE(std::isnan(empty.count));
    Float32BufferAttribute negative({1, 2}, -1);
    REQUIRE(negative.count == -2);
    REQUIRE(negative.getX(-1) == 2);
    Float32BufferAttribute fractional({1, 2, 3, 4, 5}, 2.5);
    REQUIRE(fractional.count == 2);
    REQUIRE(std::isnan(fractional.getX(1)));
    fractional.set(std::vector<float>{8}, .9);
    REQUIRE(fractional.getX(0) == 8);
    REQUIRE_THROWS_AS(fractional.set(std::vector<float>{8}, -1), JSRangeError);
}

TEST_CASE("Geometry conversion follows fractional item stride and incomplete normal triangles", "[geometry][typedarray]") {
    BufferGeometry geometry;
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1, 2, 3, 4, 5}, 2.5));
    geometry.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1}, 1));
    const auto expanded = std::dynamic_pointer_cast<BufferAttribute>(geometry.toNonIndexed()->getAttribute("position"));
    REQUIRE(std::visit([](const auto& array) { return array.size(); }, expanded->array) == 5);
    REQUIRE(expanded->getComponent(0, 2) == 3);
    REQUIRE(std::isnan(expanded->getComponent(0, 3)));
    geometry.index.reset();
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0, 4}, 3));
    geometry.computeVertexNormals();
    REQUIRE(geometry.getAttribute("normal")->getZ(0) == 1);
    REQUIRE(std::isnan(geometry.getAttribute("normal")->getX(3)));
}
