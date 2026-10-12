#include "Three.Core.h"
#include <iostream>

namespace {
struct CallbackAttribute : Float32BufferAttribute {
    using Float32BufferAttribute::Float32BufferAttribute;
    std::function<void(char, double)> onRead;
    double getX(double i) const override { if (onRead) onRead('x', i); return Float32BufferAttribute::getX(i); }
    double getY(double i) const override { if (onRead) onRead('y', i); return Float32BufferAttribute::getY(i); }
    double getZ(double i) const override { if (onRead) onRead('z', i); return Float32BufferAttribute::getZ(i); }
    double getW(double i) const override { if (onRead) onRead('w', i); return Float32BufferAttribute::getW(i); }
    BufferAttribute& setX(double, double) override { throw std::logic_error("Unexpected delegated setX"); }
};
template <class A> JsonValue values(const A& array) {
    JsonValue::Array result;
    for (const auto value : array) {
        const double number = double(value);
        if (std::isnan(number)) result.emplace_back("NaN");
        else if (std::isinf(number)) result.emplace_back(number < 0 ? "-Infinity" : "Infinity");
        else if (number == 0 && std::signbit(number)) result.emplace_back("-0");
        else result.emplace_back(number);
    }
    return result;
}
}

int main() {
    JsonValue output = JsonValue::Object{};
    Float32Array array({1, 2, 3, 4});
    const auto alias = array;
    auto view = array.subarray(1, -1);
    auto copy = view.slice();
    view[0] = 9;
    output["identity"] = JsonValue::Array{alias.sameIdentity(array), view.sameIdentity(array),
        view.buffer.sameIdentity(array.buffer), copy.buffer.sameIdentity(array.buffer)};
    output["metadata"] = JsonValue::Array{std::size_t(view.byteOffset), std::size_t(view.byteLength),
        std::size_t(view.length), Float32Array::BYTES_PER_ELEMENT};
    output["indexConversions"] = JsonValue::Array{Float32Array(2.9).size(),
        Float32Array(std::numeric_limits<double>::quiet_NaN()).size(),
        std::size_t(Uint8Array(array.buffer, 1.9, 2.9).byteOffset), Uint8Array(array.buffer, 1.9, 2.9).size()};
    output["source"] = values(array);
    output["copy"] = values(copy);
    output["emptyRanges"] = JsonValue::Array{array.subarray(3, 1).size(),
        array.subarray(-99, 99).size(), array.subarray(std::numeric_limits<double>::quiet_NaN()).size(),
        array.subarray(std::numeric_limits<double>::infinity()).size()};
    output["missing"] = JsonValue::Array{array.get(-1).isUndefined(), array.get(.5).isUndefined(),
        array.get(4).isUndefined(), array.get(-0.0).number()};
    Uint8Array overlap({1, 2, 3, 4, 5});
    overlap.set(overlap.subarray(0, 4), 1);
    output["overlap"] = values(overlap);
    Int8Array integers(8);
    integers.set(std::vector<double>{257, -129, 127.9, -128.9, 3.9, -3.9,
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()});
    output["int8"] = values(integers);
    Uint8ClampedArray clamped(10);
    clamped.set(std::vector<double>{-.5, .5, 1.5, 2.5, 3.5, 254.5, 255.5,
        std::numeric_limits<double>::quiet_NaN(), 300, -300});
    output["clamped"] = values(clamped);
    Float32Array floats(5);
    floats.set(std::vector<double>{.1, -0.0, 1e100, -1e100, std::numeric_limits<double>::quiet_NaN()});
    output["float32"] = values(floats);
    Float32Array overflow(4);
    overflow.set(std::vector<double>{double(std::numeric_limits<float>::max()) + 0x1p100,
        0x1.ffffffp127, -(double(std::numeric_limits<float>::max()) + 0x1p100), -0x1.ffffffp127});
    output["float32Overflow"] = values(overflow);
    Uint32Array nanBits({0x7f800001u, 0x80000000u});
    Float32Array nanCopy(2);
    nanCopy.set(Float32Array(nanBits.buffer));
    output["rawCopy"] = values(Uint32Array(nanCopy.buffer));
    Float32BufferAttribute converted(Uint8Array({1, 2, 3}), 3);
    Uint8ClampedBufferAttribute convertedClamp(std::vector<double>{.5, 1.5, 300}, 1);
    output["constructConversions"] = JsonValue::Array{values(std::get<Float32Array>(converted.array)),
        values(std::get<Uint8ClampedArray>(convertedClamp.array))};
    ArrayBuffer bytes(8);
    Uint32Array words(bytes);
    Float32Array reinterpreted(bytes);
    words[0] = 0x3f800000;
    reinterpreted[1] = -2;
    output["reinterpret"] = JsonValue::Array{double(float(reinterpreted[0])), double(std::uint32_t(words[1]))};
    BufferAttribute direct(view, 1);
    Float32BufferAttribute typed(view, 1);
    auto cloned = direct.clone();
    output["attributeIdentity"] = JsonValue::Array{std::get<Float32Array>(direct.array).sameIdentity(view),
        std::get<Float32Array>(typed.array).sameIdentity(view),
        std::get<Float32Array>(cloned->array).sameIdentity(view)};
    view[0] = 12;
    output["attributeValues"] = JsonValue::Array{direct.getX(0), typed.getX(0), cloned->getX(0)};
    BufferGeometry geometry;
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3}, 3));
    geometry.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 2}, 1));
    geometry.index->count = 1;
    const auto expanded = std::dynamic_pointer_cast<BufferAttribute>(geometry.toNonIndexed()->getAttribute("position"));
    output["nonIndexed"] = values(std::get<Float32Array>(expanded->array));
    geometry.setAttribute("position", std::make_shared<Uint8ClampedBufferAttribute>(std::vector<std::uint8_t>{1, 2, 3}, 3));
    const auto expandedClamped = std::dynamic_pointer_cast<BufferAttribute>(geometry.toNonIndexed()->getAttribute("position"));
    output["nonIndexedClamped"] = JsonValue::Array{expandedClamped->arrayType(), values(std::get<Uint8ClampedArray>(expandedClamped->array))};
    const auto inf = std::numeric_limits<double>::infinity();
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    JsonValue::Array numericAttributes;
    for (const double itemSize : {0.0, -0.0, -1.0, 2.5, 3.0, inf, -inf, nan}) {
        Float32BufferAttribute attribute({1, 2, 3, 4}, itemSize);
        std::vector<double> readings{itemSize, attribute.count};
        for (const double index : {-1.0, -0.0, .5, 1.0 / 3.0, 1.0, inf, nan}) {
            readings.push_back(attribute.getX(index));
            readings.push_back(attribute.getComponent(index, 1.5));
        }
        attribute.setX(.5, 21).setComponent(-1, 4, 22).setY(-0.0, 23);
        numericAttributes.emplace_back(JsonValue::Array{values(readings), values(std::get<Float32Array>(attribute.array))});
    }
    output["numericAttributes"] = std::move(numericAttributes);
    JsonValue::Array partialTransforms;
    for (int operation = 0; operation < 4; ++operation) {
        Float32BufferAttribute attribute({1, 2, 3, 4}, 3);
        if (operation == 0) attribute.applyMatrix3(Matrix3());
        if (operation == 1) attribute.applyMatrix4(Matrix4());
        if (operation == 2) attribute.applyNormalMatrix(Matrix3());
        if (operation == 3) attribute.transformDirection(Matrix4());
        partialTransforms.emplace_back(values(std::get<Float32Array>(attribute.array)));
    }
    output["partialTransforms"] = std::move(partialTransforms);
    Float32BufferAttribute copyAt({1, 2, 3, 4, 5, 6}, 2.5);
    copyAt.copyAt(.4, copyAt, 0);
    output["fractionalCopyAt"] = values(std::get<Float32Array>(copyAt.array));
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<float>{1, 2, 3, 4, 5}, 2.5));
    geometry.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0, 1}, 1));
    output["fractionalExpansion"] = values(std::get<Float32Array>(geometry.toNonIndexed()->getAttribute("position")->getArray().storage()));
    JsonValue::Array partialNormals;
    for (int indexed = 0; indexed < 2; ++indexed) {
        BufferGeometry normals;
        normals.setAttribute("position", std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0,0,0, 1,0,0, 0,1,0, 4}, 3));
        if (indexed) normals.setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2,3}, 1));
        normals.computeVertexNormals();
        partialNormals.emplace_back(values(std::get<Float32Array>(normals.getAttribute("normal")->getArray().storage())));
    }
    output["partialNormals"] = std::move(partialNormals);
    JsonValue::Array transformsWithCallbacks;
    for (int operation = 0; operation < 4; ++operation) {
        CallbackAttribute attribute({1, 2, 3, 4, 5, 6}, 3);
        Matrix3 matrix3;
        Matrix4 matrix4;
        JsonValue::Array trace;
        attribute.onRead = [&](char component, double index) {
            trace.emplace_back(std::string(1, component) + std::to_string(int(index)));
            if (component == 'x') {
                attribute.count = 0;
                matrix3.elements()[0] = matrix4.elements()[0] = 2;
            }
        };
        if (operation == 0) attribute.applyMatrix3(matrix3);
        if (operation == 1) attribute.applyMatrix4(matrix4);
        if (operation == 2) attribute.applyNormalMatrix(matrix3);
        if (operation == 3) attribute.transformDirection(matrix4);
        transformsWithCallbacks.emplace_back(JsonValue::Array{trace, values(std::get<Float32Array>(attribute.array)), attribute.count});
    }
    output["transformCallbacks"] = std::move(transformsWithCallbacks);
    CallbackAttribute callbackVector({1, 2, 3, 4}, 1);
    JsonValue::Array vectorTrace;
    callbackVector.onRead = [&](char component, double) { vectorTrace.emplace_back(std::string(1, component)); };
    Vector2 v2; Vector3 v3; Vector4 v4; Color color;
    v2.fromBufferAttribute(callbackVector, 1);
    v3.fromBufferAttribute(callbackVector, 0);
    v4.fromBufferAttribute(callbackVector, 0);
    color.fromBufferAttribute(callbackVector, 0);
    output["attributeReadOrder"] = JsonValue::Array{vectorTrace, values(v2.toArray()), values(v3.toArray()),
        values(v4.toArray()), values(color.toArray())};
    CallbackAttribute outerTransform({1, 2, 3}, 3);
    Float32BufferAttribute innerTransform({10, 20, 30}, 3);
    outerTransform.onRead = [&](char component, double) {
        if (component == 'y') innerTransform.applyMatrix4(Matrix4().makeTranslation(1, 0, 0));
    };
    outerTransform.applyMatrix4(Matrix4());
    output["transformReentry"] = JsonValue::Array{values(std::get<Float32Array>(outerTransform.array)),
        values(std::get<Float32Array>(innerTransform.array))};
    Float32BufferAttribute lifecycle({1, 2, 3}, 1);
    output["defaultUploadIdentity"] = lifecycle.onUploadCallback.value().sameValue(direct.onUploadCallback.value());
    std::vector<double> flagVersions;
    for (const auto& value : {JSValue{}, JSValue(nullptr), JSValue(false), JSValue(0), JSValue(1),
                              JSValue("true"), JSValue::object(), JSValue(true), JSValue(true)}) {
        lifecycle.needsUpdate = value;
        flagVersions.push_back(lifecycle.version.toNumber());
    }
    output["needsUpdateStrictTrue"] = values(flagVersions);
    std::vector<double> numberVersions;
    for (const double version : {.25, -2.0, 0x1p53, inf, -inf, nan}) {
        lifecycle.version = version;
        lifecycle.needsUpdate = true;
        numberVersions.push_back(lifecycle.version.toNumber());
    }
    output["numberVersions"] = values(numberVersions);
    auto rangeArray = lifecycle.updateRanges.value();
    lifecycle.addUpdateRange(.5, -2.25);
    lifecycle.addUpdateRange(nan, inf);
    auto rangeObject = rangeArray.get("0");
    output["numericRanges"] = values(std::vector<double>{rangeObject.get("start").toNumber(), rangeObject.get("count").toNumber(),
        rangeArray.get("1").get("start").toNumber(), rangeArray.get("1").get("count").toNumber()});
    const auto rangeClone = lifecycle.clone();
    lifecycle.clearUpdateRanges();
    rangeObject.set("count", 8);
    output["rangeLifecycle"] = JsonValue::Array{rangeArray.sameValue(lifecycle.updateRanges.value()), rangeArray.length() == 0,
        !rangeClone->updateRanges.value().sameValue(rangeArray), rangeClone->updateRanges.value().length() == 0, rangeObject.get("count").toNumber()};
    JsonValue::Array uploadStates;
    lifecycle.onUpload([state = 0, &uploadStates]() mutable { uploadStates.emplace_back(++state); });
    auto uploadAlias = lifecycle.onUploadCallback.value();
    uploadAlias.call(); lifecycle.invokeUploadCallback(); uploadAlias.call();
    output["uploadIdentity"] = JsonValue::Array{uploadAlias.sameValue(lifecycle.onUploadCallback.value()), uploadStates};
    BufferGeometry numericRanges;
    const auto defaultRange = values(std::vector<double>{numericRanges.drawRange.start, numericRanges.drawRange.count});
    numericRanges.addGroup(.5, 11.75, -.25);
    numericRanges.addGroup(nan, inf, -inf);
    numericRanges.setDrawRange(-.75, inf);
    const auto copiedRanges = numericRanges.clone();
    output["geometryRanges"] = JsonValue::Array{defaultRange,
        values(std::vector<double>{numericRanges.groups[0].start, numericRanges.groups[0].count,
            numericRanges.groups[0].materialIndex, numericRanges.groups[1].start,
            numericRanges.groups[1].count, numericRanges.groups[1].materialIndex,
            copiedRanges->drawRange.start, copiedRanges->drawRange.count,
            copiedRanges->groups[0].start, copiedRanges->groups[0].count})};
    JsonValue::Array errors;
    const auto error = [&](auto action) {
        try { action(); errors.emplace_back("none"); }
        catch (const JSRangeError&) { errors.emplace_back("RangeError"); }
    };
    error([&] { Float32Array invalid(bytes, 1); });
    error([&] { Float32Array invalid(bytes, 4, 2); });
    error([&] { Float32Array invalid(ArrayBuffer(3)); });
    error([&] { array.set(std::vector<float>{1}, -1); });
    error([&] { array.set(std::vector<float>{1, 2}, 3); });
    error([&] { array.set(std::vector<float>{}, std::numeric_limits<double>::infinity()); });
    error([&] { ArrayBuffer invalid(-1); });
    error([&] { Float32Array invalid(-1); });
    error([&] { Float32Array invalid(std::numeric_limits<double>::infinity()); });
    output["errors"] = std::move(errors);
    std::cout << output.dump() << '\n';
}
