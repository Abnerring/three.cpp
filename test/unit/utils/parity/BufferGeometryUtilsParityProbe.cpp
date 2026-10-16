// Differential probe for the pinned examples/jsm/utils/BufferGeometryUtils.js port.
//
// compare-buffer-geometry-utils.mjs generates both the scenarios and the expected results. The
// probe reads one JSON scenario per stdin line, builds the same attributes/geometries natively,
// replays the requested operation and prints one JSON object per line with the encoded result
// and the messages the operation wrote to stderr. The comparison script asserts both fields, so
// the console.error/console.warn text of the failure paths is part of the differential check.
#include "animation/AnimationUtils.h"
#include "core/InstancedBufferAttribute.h"
#include "core/InstancedInterleavedBuffer.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "core/SharedArray.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "utils/BufferGeometryUtils.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

using BufferGeometryUtils::AttributePtr;
using BufferGeometryUtils::GeometryPtr;
using Args = std::vector<JSValue>;

// ---------------------------------------------------------------------------
// Encoding: NaN, -0 and the infinities survive as strings.
// ---------------------------------------------------------------------------

JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}

JsonValue encodeValue(const JSValue& value) {
    if (value.isUndefined()) return "undefined";
    if (value.isNull()) return "null";
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::String) return value.toString();
    if (value.type() == JSValue::Type::Number) return number(value.toNumber());
    return value.toString();
}

// Scenario input numbers use the same string encoding for the non-finite values.
double readNumber(const JSValue& value) {
    if (value.type() == JSValue::Type::String) {
        const auto text = value.toString();
        if (text == "NaN") return std::numeric_limits<double>::quiet_NaN();
        if (text == "-0") return -0.;
        if (text == "Infinity") return std::numeric_limits<double>::infinity();
        if (text == "-Infinity") return -std::numeric_limits<double>::infinity();
    }
    return value.toNumber();
}

double readNumber(const JSValue& object, const char* name, double fallback) {
    const auto value = object.get(name);
    return value.isUndefined() ? fallback : readNumber(value);
}

// ---------------------------------------------------------------------------
// Typed array construction and inspection.
// ---------------------------------------------------------------------------

const char* const arrayTypeNames[] = {"Int8Array",  "Uint8Array",  "Int16Array",   "Uint16Array",
                                      "Int32Array", "Uint32Array", "Float32Array", "Float64Array",
                                      "Uint8ClampedArray"};

int arrayKind(const std::string& type) {
    for (int i = 0; i < 9; ++i)
        if (type == arrayTypeNames[i]) return i;
    throw std::runtime_error("unknown array type: " + type);
}

InterpolantArray newArray(int kind, double length) {
    switch (kind) {
    case 0: return Int8Array(length);
    case 1: return Uint8Array(length);
    case 2: return Int16Array(length);
    case 3: return Uint16Array(length);
    case 4: return Int32Array(length);
    case 5: return Uint32Array(length);
    case 6: return Float32Array(length);
    case 7: return Float64Array(length);
    default: return Uint8ClampedArray(length);
    }
}

std::string arrayTypeName(const InterpolantArray& array) {
    return std::holds_alternative<JSValue>(array.storage()) ? std::string("Array")
                                                            : std::string(arrayTypeNames[array.storage().index()]);
}

double arraySize(const InterpolantArray& array) { return double(array.size()); }
double arrayAt(const InterpolantArray& array, double index) { return array.get(index).toNumber(); }

InterpolantArray readArraySpec(const JSValue& spec) {
    const int kind = arrayKind(spec.get("type").toString());
    const auto values = spec.get("values");
    auto array = newArray(kind, double(values.length()));
    for (double i = 0, length = double(values.length()); i < length; ++i)
        array.set(i, JSValue(readNumber(values.get(JSValue(i).toString()))));
    return array;
}

// ---------------------------------------------------------------------------
// Attributes.
// ---------------------------------------------------------------------------

template <class Factory>
auto withTypedArray(const InterpolantArray& array, Factory factory)
    -> decltype(factory(std::declval<const Float32Array&>())) {
    return std::visit([&](const auto& values) -> decltype(factory(std::declval<const Float32Array&>())) {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, JSValue>) throw JSTypeError("BufferAttribute array must be a TypedArray");
        else return factory(values);
    }, array.storage());
}

AttributePtr buildAttribute(const JSValue& spec) {
    const auto kind = spec.get("kind").toString();
    const double itemSize = readNumber(spec, "itemSize", 1);
    const bool normalized = spec.get("normalized").toBoolean();

    if (kind == "interleaved") {
        const auto data = spec.get("data");
        auto array = readArraySpec(data);
        const double stride = readNumber(data, "stride", 1);
        JSValue buffer;
        if (data.get("instanced").toBoolean()) {
            InstancedInterleavedBuffer interleaved(nativeArray(array), JSValue(stride),
                                                   data.get("meshPerAttribute"));
            if (!data.get("usage").isUndefined()) interleaved.setUsage(data.get("usage"));
            buffer = interleaved.object();
        } else {
            InterleavedBuffer interleaved(nativeArray(array), JSValue(stride));
            if (!data.get("usage").isUndefined()) interleaved.setUsage(data.get("usage"));
            buffer = interleaved.object();
        }
        auto attribute = std::make_shared<InterleavedBufferAttribute>(buffer, JSValue(itemSize),
                                                                     spec.get("offset"), JSValue(normalized));
        if (!spec.get("name").isUndefined()) attribute->name = spec.get("name");
        return attribute;
    }

    auto array = readArraySpec(spec);
    std::shared_ptr<BufferAttribute> attribute;
    if (kind == "instanced") {
        attribute = withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<InstancedBufferAttribute>(values, itemSize, normalized,
                                                              spec.get("meshPerAttribute"));
        });
    } else if (kind == "float16") {
        attribute = withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<Float16BufferAttribute>(values, itemSize, normalized);
        });
    } else {
        attribute = withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<BufferAttribute>(values, itemSize, normalized);
        });
    }
    if (!spec.get("name").isUndefined()) attribute->name = spec.get("name").toString();
    if (!spec.get("usage").isUndefined()) attribute->setUsage(spec.get("usage"));
    if (!spec.get("gpuType").isUndefined()) attribute->gpuType = spec.get("gpuType");
    return attribute;
}

std::string attributeKind(const AttributePtr& attribute) {
    if (dynamic_cast<const InterleavedBufferAttribute*>(attribute.get()) != nullptr)
        return "InterleavedBufferAttribute";
    if (dynamic_cast<const Float16BufferAttribute*>(attribute.get()) != nullptr) return "Float16BufferAttribute";
    if (dynamic_cast<const InstancedBufferAttribute*>(attribute.get()) != nullptr)
        return "InstancedBufferAttribute";
    // The typed subclasses carry the upstream constructor names.
    if (dynamic_cast<const Int8BufferAttribute*>(attribute.get()) != nullptr) return "Int8BufferAttribute";
    if (dynamic_cast<const Uint8BufferAttribute*>(attribute.get()) != nullptr) return "Uint8BufferAttribute";
    if (dynamic_cast<const Uint8ClampedBufferAttribute*>(attribute.get()) != nullptr)
        return "Uint8ClampedBufferAttribute";
    if (dynamic_cast<const Int16BufferAttribute*>(attribute.get()) != nullptr) return "Int16BufferAttribute";
    if (dynamic_cast<const Uint16BufferAttribute*>(attribute.get()) != nullptr) return "Uint16BufferAttribute";
    if (dynamic_cast<const Int32BufferAttribute*>(attribute.get()) != nullptr) return "Int32BufferAttribute";
    if (dynamic_cast<const Uint32BufferAttribute*>(attribute.get()) != nullptr) return "Uint32BufferAttribute";
    if (dynamic_cast<const Float32BufferAttribute*>(attribute.get()) != nullptr) return "Float32BufferAttribute";
    if (dynamic_cast<const Float64BufferAttribute*>(attribute.get()) != nullptr) return "Float64BufferAttribute";
    return "BufferAttribute";
}

// The shared data identity of every attribute of one scenario. Interleaved attributes share the
// identity of their InterleavedBuffer, plain attributes are identified by themselves, exactly
// like the JavaScript comparison object keys.
struct AttributeEncoder {
    std::vector<JSValue> dataKeys;
    std::vector<const void*> objectKeys;

    JsonValue dataId(const AttributePtr& attribute) {
        if (dynamic_cast<const InterleavedBufferAttribute*>(attribute.get()) != nullptr) {
            const auto data = attribute->getProperty("data");
            for (std::size_t i = 0; i < dataKeys.size(); ++i)
                if (dataKeys[i].sameValue(data)) return double(i);
            dataKeys.push_back(data);
            return double(dataKeys.size() - 1);
        }
        for (std::size_t i = 0; i < objectKeys.size(); ++i)
            if (objectKeys[i] == attribute.get()) return double(i);
        objectKeys.push_back(attribute.get());
        return double(objectKeys.size() - 1);
    }

    JsonValue encode(const AttributePtr& attribute) {
        if (!attribute) return nullptr;
        JsonValue result = JsonValue::Object{};
        result["kind"] = attributeKind(attribute);
        const auto array = attribute->getArray();
        result["type"] = arrayTypeName(array);
        JsonValue::Array values;
        for (double i = 0, length = arraySize(array); i < length; ++i)
            values.push_back(number(arrayAt(array, i)));
        result["values"] = std::move(values);
        result["itemSize"] = number(attribute->getProperty("itemSize").toNumber());
        result["count"] = number(attribute->getProperty("count").toNumber());
        result["normalized"] = attribute->getProperty("normalized").toBoolean();
        result["name"] = attribute->getProperty("name").toString();
        result["usage"] = encodeValue(attribute->getProperty("usage"));
        result["gpuType"] = encodeValue(attribute->getProperty("gpuType"));
        result["dataId"] = dataId(attribute);
        if (dynamic_cast<const InterleavedBufferAttribute*>(attribute.get()) != nullptr) {
            const auto data = attribute->getProperty("data");
            result["offset"] = number(attribute->getProperty("offset").toNumber());
            result["stride"] = number(data.get("stride").toNumber());
            result["dataCount"] = number(data.get("count").toNumber());
            result["dataUsage"] = encodeValue(data.get("usage"));
            result["dataKind"] = data.get("isInstancedInterleavedBuffer").toBoolean()
                                     ? "InstancedInterleavedBuffer"
                                     : "InterleavedBuffer";
        }
        if (dynamic_cast<const InstancedBufferAttribute*>(attribute.get()) != nullptr)
            result["meshPerAttribute"] =
                encodeValue(std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute)->meshPerAttribute);
        return result;
    }
};

// ---------------------------------------------------------------------------
// Geometries.
// ---------------------------------------------------------------------------

GeometryPtr buildGeometry(const JSValue& spec) {
    auto geometry = std::make_shared<BufferGeometry>();

    const auto attributes = spec.get("attributes");
    if (!attributes.isUndefined())
        for (const auto& name : attributes.keys())
            geometry->setAttribute(name, buildAttribute(attributes.get(name)));

    const auto index = spec.get("index");
    if (!index.isUndefined() && !index.isNull())
        geometry->setIndex(std::dynamic_pointer_cast<BufferAttribute>(buildAttribute(index)));

    const auto morphs = spec.get("morphAttributes");
    if (!morphs.isUndefined())
        for (const auto& name : morphs.keys()) {
            const auto list = morphs.get(name);
            for (double i = 0, length = double(list.length()); i < length; ++i)
                geometry->morphAttributes[name].push_back(buildAttribute(list.get(JSValue(i).toString())));
        }

    geometry->morphTargetsRelative = spec.get("morphTargetsRelative").toBoolean();

    const auto groups = spec.get("groups");
    if (!groups.isUndefined())
        for (double i = 0, length = double(groups.length()); i < length; ++i) {
            const auto group = groups.get(JSValue(i).toString());
            geometry->addGroup(readNumber(group, "start", 0), readNumber(group, "count", 0),
                               readNumber(group, "materialIndex", 0));
        }

    const auto drawRange = spec.get("drawRange");
    if (!drawRange.isUndefined())
        geometry->drawRange = {readNumber(drawRange, "start", 0),
                               readNumber(drawRange, "count", std::numeric_limits<double>::infinity())};

    return geometry;
}

JsonValue encodeGeometry(const GeometryPtr& geometry, AttributeEncoder& encoder) {
    if (!geometry) return nullptr;
    JsonValue result = JsonValue::Object{};

    JsonValue::Object attributes;
    for (const auto& [name, attribute] : geometry->attributes) attributes[name] = encoder.encode(attribute);
    result["attributes"] = std::move(attributes);

    JsonValue::Object morphAttributes;
    for (const auto& [name, list] : geometry->morphAttributes) {
        JsonValue::Array encoded;
        for (const auto& attribute : list) encoded.push_back(encoder.encode(attribute));
        morphAttributes[name] = std::move(encoded);
    }
    result["morphAttributes"] = std::move(morphAttributes);
    result["morphTargetsRelative"] = geometry->morphTargetsRelative;
    result["index"] = geometry->getIndex() ? encoder.encode(geometry->getIndex()) : JsonValue(nullptr);

    JsonValue::Array groups;
    for (const auto& group : geometry->groups)
        groups.push_back(JsonValue::Object{{"start", number(group.start)},
                                           {"count", number(group.count)},
                                           {"materialIndex", number(group.materialIndex)}});
    result["groups"] = std::move(groups);
    result["drawRange"] = JsonValue::Object{{"start", number(geometry->drawRange.start)},
                                            {"count", number(geometry->drawRange.count)}};
    return result;
}

JsonValue encodeAttributes(const std::vector<AttributePtr>& attributes, AttributeEncoder& encoder) {
    if (attributes.empty()) return nullptr;
    JsonValue::Array encoded;
    for (const auto& attribute : attributes) encoded.push_back(encoder.encode(attribute));
    return encoded;
}

// ---------------------------------------------------------------------------
// Scenarios.
// ---------------------------------------------------------------------------

JsonValue run(const JSValue& spec, AttributeEncoder& encoder) {
    const auto kind = spec.get("kind").toString();

    if (kind == "mergeGeometries") {
        const auto list = spec.get("geometries");
        BufferGeometryUtils::GeometryList geometries;
        for (double i = 0, length = double(list.length()); i < length; ++i)
            geometries.push_back(buildGeometry(list.get(JSValue(i).toString())));
        return encodeGeometry(BufferGeometryUtils::mergeGeometries(geometries, spec.get("useGroups").toBoolean()),
                              encoder);
    }
    if (kind == "mergeAttributes") {
        const auto list = spec.get("attributes");
        BufferGeometryUtils::AttributeList attributes;
        for (double i = 0, length = double(list.length()); i < length; ++i)
            attributes.push_back(buildAttribute(list.get(JSValue(i).toString())));
        return encoder.encode(BufferGeometryUtils::mergeAttributes(attributes));
    }
    if (kind == "deepCloneAttribute")
        return encoder.encode(BufferGeometryUtils::deepCloneAttribute(buildAttribute(spec.get("attribute"))));
    if (kind == "interleaveAttributes") {
        const auto list = spec.get("attributes");
        BufferGeometryUtils::AttributeList attributes;
        for (double i = 0, length = double(list.length()); i < length; ++i)
            attributes.push_back(buildAttribute(list.get(JSValue(i).toString())));
        return encodeAttributes(BufferGeometryUtils::interleaveAttributes(attributes), encoder);
    }
    if (kind == "deinterleaveAttribute")
        return encoder.encode(BufferGeometryUtils::deinterleaveAttribute(buildAttribute(spec.get("attribute"))));
    if (kind == "deinterleaveGeometry") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        BufferGeometryUtils::deinterleaveGeometry(*geometry);
        return encodeGeometry(geometry, encoder);
    }
    if (kind == "estimateBytesUsed")
        return number(BufferGeometryUtils::estimateBytesUsed(*buildGeometry(spec.get("geometry"))));
    if (kind == "mergeVertices") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        return encodeGeometry(BufferGeometryUtils::mergeVertices(geometry, readNumber(spec, "tolerance", 1e-4)),
                              encoder);
    }
    if (kind == "toTrianglesDrawMode") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        const auto result = BufferGeometryUtils::toTrianglesDrawMode(geometry, readNumber(spec, "drawMode", 0));
        return JsonValue::Object{{"sameGeometry", result == geometry},
                                 {"geometry", encodeGeometry(result, encoder)}};
    }
    if (kind == "mergeGroups") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        const auto result = BufferGeometryUtils::mergeGroups(geometry);
        return JsonValue::Object{{"sameGeometry", result == geometry}, {"geometry", encodeGeometry(result, encoder)}};
    }
    if (kind == "toCreasedNormals") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        const auto result =
            BufferGeometryUtils::toCreasedNormals(geometry, readNumber(spec, "creaseAngle", 1.0471975511965976));
        return JsonValue::Object{{"sameGeometry", result == geometry}, {"geometry", encodeGeometry(result, encoder)}};
    }
    if (kind == "computeMorphedAttributes") {
        const auto geometry = buildGeometry(spec.get("geometry"));
        auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
        mesh->materialIsArray = spec.get("materialArray").toBoolean();
        const auto influences = spec.get("influences");
        if (!influences.isUndefined()) {
            std::vector<double> values;
            for (double i = 0, length = double(influences.length()); i < length; ++i)
                values.push_back(readNumber(influences.get(JSValue(i).toString())));
            mesh->morphTargetInfluences = SharedArray<double>(values);
        }
        const auto result = BufferGeometryUtils::computeMorphedAttributes(mesh);
        return JsonValue::Object{
            {"positionAttribute", encoder.encode(result.positionAttribute)},
            {"normalAttribute", encoder.encode(result.normalAttribute)},
            {"morphedPositionAttribute", encoder.encode(result.morphedPositionAttribute)},
            {"morphedNormalAttribute", encoder.encode(result.morphedNormalAttribute)}};
    }
    if (kind == "remapAttribute") {
        const auto attribute = buildAttribute(spec.get("attribute"));
        const auto indices = spec.get("sourceIndices");
        std::vector<double> values;
        for (double i = 0, length = double(indices.length()); i < length; ++i)
            values.push_back(readNumber(indices.get(JSValue(i).toString())));
        const AttributePtr sourceIndices = std::make_shared<Float64BufferAttribute>(values, 1.);
        std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>> buffers;
        return encoder.encode(BufferGeometryUtils::remapAttribute(attribute, sourceIndices, buffers));
    }
    if (kind == "remapAttributes") {
        // One pass over several attributes with the shared `buffers` cache of the source call.
        const auto list = spec.get("attributes");
        const auto indexLists = spec.get("sourceIndices");
        std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>> buffers;
        JsonValue::Array encoded;
        for (double i = 0, length = double(list.length()); i < length; ++i) {
            const auto attribute = buildAttribute(list.get(JSValue(i).toString()));
            const auto indices = indexLists.get(JSValue(i).toString());
            std::vector<double> values;
            for (double j = 0, count = double(indices.length()); j < count; ++j)
                values.push_back(readNumber(indices.get(JSValue(j).toString())));
            const AttributePtr sourceIndices = std::make_shared<Float64BufferAttribute>(values, 1.);
            encoded.push_back(encoder.encode(
                BufferGeometryUtils::remapAttribute(attribute, sourceIndices, buffers)));
        }
        return encoded;
    }
    if (kind == "computeMikkTSpaceTangents") {
        // The MikkTSpace library is missing on both sides, so the entry point throws.
        return encodeGeometry(BufferGeometryUtils::computeMikkTSpaceTangents(buildGeometry(spec.get("geometry"))),
                              encoder);
    }
    throw std::runtime_error("unknown scenario kind: " + kind);
}

JsonValue errorName(const std::exception& error) {
    if (dynamic_cast<const JSTypeError*>(&error) != nullptr)
        // The port keeps the V8 message text of a missing attribute, so it is compared.
        return JsonValue::Object{{"name", std::string("TypeError")}, {"message", std::string(error.what())}};
    if (dynamic_cast<const JSRangeError*>(&error) != nullptr)
        return JsonValue::Object{{"name", std::string("RangeError")}};
    return JsonValue::Object{{"name", std::string("Error")}};
}

} // namespace

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        AttributeEncoder encoder;
        std::ostringstream messages;
        auto* before = std::cerr.rdbuf(messages.rdbuf());
        JsonValue value;
        try {
            value = run(Animation_detail::parseJSON(line), encoder);
        } catch (const std::exception& error) {
            value = JsonValue::Object{{"error", errorName(error)}};
        }
        std::cerr.rdbuf(before);
        JsonValue::Array log;
        std::istringstream lines(messages.str());
        for (std::string entry; std::getline(lines, entry);) log.emplace_back(entry);
        std::cout << JsonValue(JsonValue::Object{{"result", value}, {"messages", log}}).dump() << '\n';
    }
    return 0;
}
