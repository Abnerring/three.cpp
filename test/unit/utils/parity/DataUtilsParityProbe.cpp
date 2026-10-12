#include "core/BufferAttribute.h"
#include "extras/DataUtils.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
template <class A> JsonValue values(const A& array) {
    JsonValue::Array result;
    for (const auto value : array) result.push_back(number(double(value)));
    return result;
}
JsonValue state(const BufferAttribute& attribute) {
    JsonValue::Array readings;
    for (double i = 0; i < attribute.count; ++i)
        readings.emplace_back(values(std::vector<double>{attribute.getX(i), attribute.getY(i),
            attribute.getZ(i), attribute.getW(i), attribute.getComponent(i, 0)}));
    return JsonValue::Object{{"array", std::visit([](const auto& a) { return values(a); }, attribute.array)},
        {"readings", readings}, {"count", number(attribute.count)}, {"itemSize", number(attribute.itemSize)},
        {"type", attribute.arrayType()}, {"half", attribute.isFloat16BufferAttribute.toBoolean()},
        {"normalized", attribute.normalized}};
}
template <class F> std::string errorName(F action) {
    try { action(); return "none"; }
    catch (const JSRangeError&) { return "RangeError"; }
    catch (const std::exception&) { return "Error"; }
}
template <class F> JsonValue capture(F action) {
    try { return action(); }
    catch (const std::exception&) { return "Error"; }
}
template <class Attribute, class Array> JsonValue bufferConstructor() {
    ArrayBuffer bytes(16);
    Array external(bytes);
    Attribute first(bytes, 2), second(bytes, 2);
    const auto& firstView = std::get<Array>(first.array);
    const auto& secondView = std::get<Array>(second.array);
    external[0] = 0x3c00;
    second.setY(0, 2.5);
    auto cloned = first.clone();
    first.setX(0, 3.5);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    return JsonValue::Object{
        {"identity", JsonValue::Array{firstView.buffer.sameIdentity(bytes), secondView.buffer.sameIdentity(bytes),
            firstView.sameIdentity(secondView), firstView.sameIdentity(external),
            std::get<Array>(cloned->array).buffer.sameIdentity(bytes)}},
        {"metadata", JsonValue::Array{std::size_t(firstView.byteOffset), std::size_t(firstView.byteLength),
            firstView.size(), first.count}},
        {"readings", values(std::vector<double>{first.getX(0), second.getX(0), cloned->getX(0), first.getY(0)})},
        {"raw", values(external)},
        {"lengths", JsonValue::Array{Attribute(3.9, 2).count, Attribute(nan, 1).count,
            Attribute(ArrayBuffer(0), 1).count}},
        {"errors", JsonValue::Array{errorName([] { Attribute invalid(-1., 1); }),
            errorName([] { Attribute invalid(std::numeric_limits<double>::infinity(), 1); }),
            errorName([] { Attribute invalid(ArrayBuffer(3), 1); })}}};
}
}

int main() {
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    JsonValue output = JsonValue::Object{};
    JsonValue::Array decoded, encoded;
    for (unsigned bits = 0; bits < 65536; ++bits) decoded.push_back(number(fromHalfFloat(bits)));
    output["decoded"] = std::move(decoded);
    // Values immediately surrounding every finite positive half at Float32 precision,
    // with both signs, exercise the entire table and truncation transitions.
    for (unsigned half = 0; half <= 0x7bff; ++half) {
        const auto bits = std::bit_cast<std::uint32_t>(float(fromHalfFloat(half)));
        for (int delta : {-1, 0, 1}) {
            const auto next = std::uint32_t(std::max(std::int64_t(0), std::int64_t(bits) + delta));
            const auto value = double(std::bit_cast<float>(next));
            encoded.emplace_back(toHalfFloat(value));
            encoded.emplace_back(toHalfFloat(-value));
        }
    }
    output["encodedBoundaries"] = std::move(encoded);
    const std::vector<double> conversions{0., -0., 1., -1., 3.141592653589793, -3.141592653589793,
        65504., -65504., 65519., -65519., 1e300, -1e300, inf, -inf, nan,
        0x1p-24, 0x1p-25, -0x1p-25, 1 + 0x1p-11, 1 + 0x1p-10 - 0x1p-25,
        1 + 0x1p-10 - 0x1p-24, std::numeric_limits<double>::denorm_min()};
    JsonValue::Array converted;
    for (const auto value : conversions) converted.emplace_back(DataUtils::toHalfFloat(value));
    output["converted"] = std::move(converted);
    const std::vector<std::uint64_t> nanBits{0x7ff0000000000001ull, 0x7ff8000000000000ull,
        0x7ff8123456789abcull, 0x7fffffffffffffffull, 0xfff0000000000001ull,
        0xfff8000000000000ull, 0xfff8123456789abcull, 0xffffffffffffffffull};
    JsonValue::Array nanPayloads;
    for (unsigned round = 0; round < 4; ++round) {
        JsonValue::Array outputs;
        for (const auto bits : nanBits) outputs.emplace_back(toHalfFloat(std::bit_cast<double>(bits)));
        nanPayloads.emplace_back(std::move(outputs));
        for (unsigned i = 0; i < 100000; ++i)
            toHalfFloat(i % 2 ? std::bit_cast<double>(nanBits[i % nanBits.size()]) : 1.25);
    }
    output["nanPayloads"] = std::move(nanPayloads);
    JsonValue::Array invalid;
    for (const auto value : std::vector<double>{-0., -1., -1024., -65536., 65535.9, 65536.,
         15360.9, 2147483647., -2147483648., 4294967296. + 15360., -4294967296. + 15360.,
         9007199254740991., -9007199254740991., inf, -inf, nan})
        invalid.push_back(number(DataUtils::fromHalfFloat(value)));
    output["invalid"] = std::move(invalid);

    Uint16Array raw({0x3c00, 0x4000, 0x4200, 0x8000});
    Float16BufferAttribute constructed(raw, 2);
    output["constructorAlias"] = std::get<Uint16Array>(constructed.array).sameIdentity(raw);
    raw[0] = 0x4400;
    output["constructed"] = state(constructed);
    Float16BufferAttribute convertedRaw(std::vector<double>{1, -1, 65537, 1.9, nan, inf}, 2);
    output["rawConversions"] = state(convertedRaw);
    Float16BufferAttribute floatInput(Float32Array({15360, 16384, 16896}), 3);
    output["typedConversions"] = state(floatInput);
    output["lengths"] = JsonValue::Array{state(Float16BufferAttribute(3.9, 2)),
        state(Float16BufferAttribute(nan, 2))};
    output["lengthErrors"] = JsonValue::Array{
        errorName([] { Float16BufferAttribute attribute(-1., 1); }),
        errorName([&] { Float16BufferAttribute attribute(inf, 1); })};
    output["bufferConstructors"] = JsonValue::Array{
        bufferConstructor<Float16BufferAttribute, Uint16Array>(),
        bufferConstructor<Int8BufferAttribute, Int8Array>(),
        bufferConstructor<Uint8BufferAttribute, Uint8Array>(),
        bufferConstructor<Int16BufferAttribute, Int16Array>(),
        bufferConstructor<Uint16BufferAttribute, Uint16Array>(),
        bufferConstructor<Int32BufferAttribute, Int32Array>(),
        bufferConstructor<Uint32BufferAttribute, Uint32Array>(),
        bufferConstructor<Float32BufferAttribute, Float32Array>(),
        bufferConstructor<Uint8ClampedBufferAttribute, Uint8ClampedArray>()};

    Uint16Array storage({0, 0x3c00, 0x4000, 0});
    auto view = storage.subarray(1, 3);
    Float16BufferAttribute assigned(2., 1);
    assigned.array = view;
    assigned.name = "half";
    assigned.usage = DynamicDrawUsage;
    assigned.gpuType = IntType;
    assigned.needsUpdate = true;
    assigned.addUpdateRange(0, 1);
    auto clone = assigned.clone();
    output["cloneMetadata"] = JsonValue::Array{bool(std::dynamic_pointer_cast<Float16BufferAttribute>(clone)),
        clone->name, clone->usage.toNumber(), clone->gpuType.toNumber(), clone->version.toNumber(), clone->updateRanges.value().length(),
        std::get<Uint16Array>(clone->array).buffer.sameIdentity(storage.buffer)};
    assigned.setX(0, 3);
    clone->setX(1, 4);
    output["sharedView"] = JsonValue::Array{values(storage), state(assigned), state(*clone)};
    Float16BufferAttribute setters(8., 4);
    BufferAttribute& base = setters;
    output["setterReturn"] = &base.setXYZW(0, 1, 2, 3, 4) == &base;
    base.setXY(1, 5, 6).setZ(1, 7).setW(1, 8);
    base.setX(0, -0.).setY(0, inf).setZ(0, -inf).setW(0, nan);
    output["setters"] = state(setters);
    Float16BufferAttribute normalized(8., 4, true);
    normalized.setXYZW(0, .5, 1, -0., -.5).setXYZ(1, .25, 0, -1).setW(1, nan);
    output["normalized"] = state(normalized);
    normalized.setComponent(0, 0, double(0x3c00) / 65535.);
    output["genericNormalized"] = state(normalized);
    normalized.copyArray(std::vector<std::uint16_t>{0x4000, 0x4200});
    normalized.copyAt(1, normalized, 0);
    output["rawCopy"] = state(normalized);

    JsonValue::Array partial;
    for (const auto itemSize : {0., -1., 2., 2.5}) {
        Float16BufferAttribute attribute(std::vector<std::uint16_t>{0x3c00, 0x4000, 0x4200}, itemSize);
        JsonValue::Array readings{number(attribute.count)};
        for (const auto index : std::vector<double>{-1., -0., .25, .5, 1., inf, nan}) {
            readings.push_back(number(attribute.getX(index)));
            readings.push_back(number(attribute.getY(index)));
            readings.push_back(number(attribute.getComponent(index, 0)));
        }
        attribute.setX(.25, 9).setY(1, 9).setX(-1, 9);
        partial.emplace_back(JsonValue::Array{readings, values(std::get<Uint16Array>(attribute.array))});
    }
    output["numericIndices"] = std::move(partial);
    JsonValue::Array transformed;
    for (int operation = 0; operation < 5; ++operation) {
        const double size = operation == 4 ? 2 : 3;
        Float16BufferAttribute attribute(std::vector<std::uint16_t>{0x3c00, 0x4000, 0x4200, 0x4400}, size);
        BufferAttribute& polymorphic = attribute;
        if (operation == 0) polymorphic.applyMatrix3(Matrix3().set(2, 1, 3, -1, 3, 2, 0, 2, 4));
        if (operation == 1) polymorphic.applyMatrix4(Matrix4().makeTranslation(4, -2, 1));
        if (operation == 2) polymorphic.applyNormalMatrix(Matrix3().set(2, 1, 0, 0, 3, 0, 1, 0, 4));
        if (operation == 3) polymorphic.transformDirection(Matrix4().makeScale(2, 3, -4));
        if (operation == 4) polymorphic.applyMatrix3(Matrix3().set(2, 1, 3, -1, 3, 2, 0, 2, 4));
        transformed.push_back(state(attribute));
    }
    output["transforms"] = std::move(transformed);
    Float16BufferAttribute copied(0., 1);
    Float32BufferAttribute source(std::vector<float>{15360, 16384, 16896}, 3, true);
    copied.copy(source);
    copied.setXYZ(0, 1, 2, 3);
    output["copyOtherType"] = state(copied);
    const std::vector<int> rawValues{0x3c00, 0x4000, 0x4200, 0x4400};
    const std::vector<BufferAttribute::Array> sourceArrays{
        Int8Array(rawValues), Uint8Array(rawValues), Int16Array(rawValues), Uint16Array(rawValues),
        Int32Array(rawValues), Uint32Array(rawValues), Float32Array(rawValues), Float64Array(rawValues),
        Uint8ClampedArray(rawValues)};
    JsonValue::Array reboundTypes;
    for (const auto& array : sourceArrays) {
        for (const auto normalize : {false, true}) {
            auto source = std::visit([&](const auto& a) {
                return std::make_shared<BufferAttribute>(a, 4, normalize);
            }, array);
            Float16BufferAttribute destination(0., 4);
            destination.copy(*source);
            const auto before = capture([&] { return state(destination); });
            const auto write = errorName([&] { destination.setXYZW(0, .5, -.5, -0., nan); });
            reboundTypes.emplace_back(JsonValue::Array{before, write,
                capture([&] { return state(destination); }),
                std::visit([](const auto& a) { return values(a); }, destination.array)});
        }
    }
    output["reboundTypes"] = std::move(reboundTypes);
    output["serialized"] = constructed.toJSON();
    std::cout << output.dump() << '\n';
}
