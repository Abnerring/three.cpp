#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "extras/DataUtils.h"
#include <cmath>
#include <limits>

TEST_CASE("DataUtils decodes every binary16 bit pattern", "[data-utils]") {
    for (unsigned bits = 0; bits < 65536; ++bits) {
        const unsigned exponent = (bits >> 10) & 31, fraction = bits & 1023;
        const auto actual = fromHalfFloat(bits);
        if (exponent == 31 && fraction) {
            REQUIRE(std::isnan(actual));
        } else {
            const double magnitude = exponent == 31 ? std::numeric_limits<double>::infinity() :
                exponent == 0 ? std::ldexp(double(fraction), -24) :
                std::ldexp(double(1024 + fraction), int(exponent) - 25);
            REQUIRE(actual == (bits & 0x8000 ? -magnitude : magnitude));
            REQUIRE(std::signbit(actual) == bool(bits & 0x8000));
        }
    }
}

TEST_CASE("DataUtils preserves signed zero and truncates after Float32 conversion", "[data-utils]") {
    REQUIRE(toHalfFloat(-0.0) == 0x8000);
    REQUIRE(std::signbit(DataUtils::fromHalfFloat(0x8000)));
    REQUIRE(DataUtils::toHalfFloat(3.141592653589793) == 0x4248);
    REQUIRE(toHalfFloat(1 + std::ldexp(1., -11)) == 0x3c00);
    REQUIRE(toHalfFloat(1 + std::ldexp(1., -10) - std::ldexp(1., -25)) == 0x3c01);
    REQUIRE(toHalfFloat(std::ldexp(1., -24)) == 1);
    REQUIRE(toHalfFloat(std::ldexp(1., -25)) == 0);
    REQUIRE(toHalfFloat(-std::ldexp(1., -25)) == 0x8000);
    REQUIRE(toHalfFloat(std::numeric_limits<double>::infinity()) == 0x7bff);
    REQUIRE(toHalfFloat(-std::numeric_limits<double>::infinity()) == 0xfbff);
    REQUIRE(toHalfFloat(std::numeric_limits<double>::quiet_NaN()) == 0xfe00);
    REQUIRE(toHalfFloat(-std::numeric_limits<double>::quiet_NaN()) == 0xfe00);
}

TEST_CASE("DataUtils half input follows bitwise conversion instead of Uint16 clamping", "[data-utils]") {
    REQUIRE(fromHalfFloat(-1) == 0);
    REQUIRE_FALSE(std::signbit(fromHalfFloat(-1)));
    REQUIRE(fromHalfFloat(65536) == 0);
    REQUIRE(fromHalfFloat(4294967296. + 15360) == 1);
    REQUIRE(fromHalfFloat(-4294967296. + 15360) == 1);
    REQUIRE(fromHalfFloat(15360.9) == 1);
    REQUIRE(fromHalfFloat(std::numeric_limits<double>::quiet_NaN()) == 0);
    REQUIRE(fromHalfFloat(std::numeric_limits<double>::infinity()) == 0);
}

TEST_CASE("Float16BufferAttribute constructor copies raw Uint16 values", "[data-utils]") {
    Uint16Array input({0x3c00, 0x4000, 0x4200, 0x8000});
    Float16BufferAttribute attribute(input, 2);
    REQUIRE(attribute.isFloat16BufferAttribute.toBoolean());
    REQUIRE(attribute.arrayType() == "Uint16Array");
    REQUIRE(attribute.count == 2);
    REQUIRE_FALSE(std::get<Uint16Array>(attribute.array).sameIdentity(input));
    input[0] = 0x4400;
    REQUIRE(attribute.getX(0) == 1);
    REQUIRE(attribute.getY(0) == 2);
    REQUIRE(std::signbit(attribute.getY(1)));
    REQUIRE(attribute.getComponent(0, 0) == 0x3c00);
    attribute.setComponent(0, 0, 0x4500);
    REQUIRE(attribute.getX(0) == 5);
    Float16BufferAttribute numeric(std::vector<double>{1, -1, 65537, 1.9}, 1);
    REQUIRE(numeric.getComponent(0, 0) == 1);
    REQUIRE(numeric.getComponent(1, 0) == 65535);
    REQUIRE(numeric.getComponent(2, 0) == 1);
    REQUIRE(numeric.getComponent(3, 0) == 1);
}

TEST_CASE("Float16BufferAttribute shares assigned views but clone owns independent storage", "[data-utils]") {
    Uint16Array storage({0, 0x3c00, 0x4000, 0});
    auto view = storage.subarray(1, 3);
    Float16BufferAttribute attribute(2., 1);
    attribute.array = view;
    attribute.name = "half";
    attribute.setUsage(DynamicDrawUsage);
    attribute.needsUpdate = true;
    attribute.addUpdateRange(0, 1);
    auto clone = attribute.clone();
    REQUIRE(std::dynamic_pointer_cast<Float16BufferAttribute>(clone));
    REQUIRE(clone->name == "half");
    REQUIRE(clone->usage.toNumber() == DynamicDrawUsage);
    REQUIRE(clone->version.toNumber() == 0);
    REQUIRE(clone->updateRanges.value().length() == 0);
    REQUIRE_FALSE(std::get<Uint16Array>(clone->array).buffer.sameIdentity(storage.buffer));
    attribute.setX(0, 3);
    REQUIRE(std::uint16_t(storage[1]) == 0x4200);
    REQUIRE(clone->getX(0) == 1);
    clone->setX(0, 4);
    REQUIRE(attribute.getX(0) == 3);
}

TEST_CASE("Float16BufferAttribute ArrayBuffer construction shares bytes with independent views", "[data-utils]") {
    ArrayBuffer bytes(8);
    Uint16Array words(bytes);
    Float16BufferAttribute first(bytes, 2), second(bytes, 2);
    auto& firstView = std::get<Uint16Array>(first.array);
    auto& secondView = std::get<Uint16Array>(second.array);
    REQUIRE(firstView.buffer.sameIdentity(bytes));
    REQUIRE(secondView.buffer.sameIdentity(bytes));
    REQUIRE_FALSE(firstView.sameIdentity(secondView));
    REQUIRE_FALSE(firstView.sameIdentity(words));
    REQUIRE(std::size_t(firstView.byteOffset) == 0);
    REQUIRE(std::size_t(firstView.byteLength) == 8);
    words[0] = 0x3c00;
    REQUIRE(first.getX(0) == 1);
    second.setY(0, 2);
    REQUIRE(first.getY(0) == 2);
    REQUIRE(std::uint16_t(words[1]) == 0x4000);
    auto copied = first.clone();
    words[0] = 0x4200;
    REQUIRE(copied->getX(0) == 1);
    REQUIRE(first.getX(0) == 3);
    REQUIRE_THROWS_AS(Float16BufferAttribute(ArrayBuffer(3), 1), JSRangeError);
    REQUIRE(Float16BufferAttribute(ArrayBuffer(0), 1).count == 0);
    REQUIRE(Float16BufferAttribute({0x3c00}, 1).getX(0) == 1);
}

TEST_CASE("TypedBufferAttribute ArrayBuffer and numeric length constructors retain TypedArray semantics", "[data-utils]") {
    ArrayBuffer bytes(16);
    Float32Array floats(bytes);
    Float32BufferAttribute first(bytes, 2), second(bytes, 2);
    REQUIRE(std::get<Float32Array>(first.array).buffer.sameIdentity(bytes));
    REQUIRE_FALSE(std::get<Float32Array>(first.array).sameIdentity(std::get<Float32Array>(second.array)));
    floats[0] = 1.5;
    REQUIRE(first.getX(0) == 1.5);
    second.setY(0, 2.5);
    REQUIRE(first.getY(0) == 2.5);
    REQUIRE(Float32BufferAttribute(3.9, 2).count == 1.5);
    REQUIRE(Float32BufferAttribute(std::numeric_limits<double>::quiet_NaN(), 1).count == 0);
    REQUIRE_THROWS_AS(Float32BufferAttribute(-1., 1), JSRangeError);
    REQUIRE_THROWS_AS(Float32BufferAttribute(std::numeric_limits<double>::infinity(), 1), JSRangeError);
    REQUIRE_THROWS_AS(Float32BufferAttribute(ArrayBuffer(3), 1), JSRangeError);
    REQUIRE(Uint8BufferAttribute(ArrayBuffer(3), 1).count == 3);
    REQUIRE_THROWS_AS(Float64BufferAttribute(ArrayBuffer(4), 1), JSRangeError);
    REQUIRE(Float64BufferAttribute({.5}, 1).getX(0) == .5);
}

TEST_CASE("Float16BufferAttribute virtual setters and transforms encode half values", "[data-utils]") {
    Float16BufferAttribute attribute(8., 4);
    BufferAttribute& base = attribute;
    REQUIRE(&base.setXYZW(0, 1, 2, 3, 4) == &base);
    base.setXY(1, 5, 6).setZ(1, 7).setW(1, 8);
    REQUIRE(attribute.getW(1) == 8);
    REQUIRE(attribute.getComponent(0, 2) == 0x4200);
    Float16BufferAttribute positions(std::vector<std::uint16_t>{0x3c00, 0x4000, 0x4200}, 3);
    BufferAttribute& basePositions = positions;
    basePositions.applyMatrix4(Matrix4().makeTranslation(4, -2, 1));
    REQUIRE(positions.getX(0) == 5);
    REQUIRE(positions.getY(0) == 0);
    REQUIRE(positions.getZ(0) == 4);
    basePositions.applyMatrix3(Matrix3().set(2, 0, 0, 0, 3, 0, 0, 0, 4));
    REQUIRE(positions.getX(0) == 10);
    REQUIRE(positions.getZ(0) == 16);
}

TEST_CASE("Float16BufferAttribute normalized component APIs keep distinct raw semantics", "[data-utils]") {
    Float16BufferAttribute attribute(4., 4, true);
    attribute.setXYZW(0, .5, 1, -0., -.5);
    REQUIRE(attribute.getX(0) == 32768. / 65535.);
    REQUIRE(attribute.getY(0) == 65504. / 65535.);
    REQUIRE(std::signbit(attribute.getZ(0)));
    REQUIRE(attribute.getComponent(0, 0) == double(0x7800) / 65535.);
    attribute.setComponent(0, 0, double(0x3c00) / 65535.);
    REQUIRE(attribute.getX(0) == 1. / 65535.);
}

TEST_CASE("Float16BufferAttribute missing samples decode zero and invalid writes are ignored", "[data-utils]") {
    Float16BufferAttribute attribute(std::vector<std::uint16_t>{0x3c00, 0x4000, 0x4200}, 2);
    REQUIRE(attribute.count == 1.5);
    REQUIRE(attribute.getX(.5) == 2);
    REQUIRE(attribute.getY(1) == 0);
    REQUIRE(attribute.getX(-1) == 0);
    REQUIRE(attribute.getX(.25) == 0);
    REQUIRE(attribute.getX(std::numeric_limits<double>::quiet_NaN()) == 0);
    REQUIRE(std::isnan(attribute.getComponent(-1, 0)));
    attribute.setX(.25, 9).setY(1, 9).setX(-1, 9);
    REQUIRE(attribute.getX(0) == 1);
    REQUIRE(attribute.getY(0) == 2);
    REQUIRE(attribute.getX(1) == 3);
    REQUIRE_THROWS_AS(Float16BufferAttribute(-1., 1), JSRangeError);
}
