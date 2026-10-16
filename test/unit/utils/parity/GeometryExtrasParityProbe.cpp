// Differential probe for the pinned addon geometry modules:
// examples/jsm/geometries/ParametricFunctions.js, ParametricGeometry.js, BoxLineGeometry.js and
// RoundedBoxGeometry.js.
//
// Protocol: one scenario per stdin line, tokens separated by spaces.
//
//   F <funcId> <u> <v>                    ParametricFunctions.<fn>( u, v, target )
//   P <funcId> <slices> <stacks>          new ParametricGeometry( fn, slices, stacks )
//   B <w> <h> <d> <ws> <hs> <ds>          new BoxLineGeometry( w, h, d, ws, hs, ds )
//   R <w> <h> <d> <segments> <radius>     new RoundedBoxGeometry( w, h, d, segments, radius )
//
// funcId: 0 default parametric function, 1 plane, 2 klein, 3 mobius, 4 mobius3d.
// Attribute dumps carry the component values in order; the JSON field of a geometry scenario
// exposes the parameters the pinned BufferGeometry#toJSON copies into its result.
#include "core/BufferAttribute.h"
#include "core/JsonValue.h"
#include "geometries/BoxLineGeometry.h"
#include "geometries/ParametricFunctions.h"
#include "geometries/ParametricGeometry.h"
#include "geometries/RoundedBoxGeometry.h"
#include "math/Vector3.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue arrayJSON(const InterpolantArray& array) {
    JsonValue::Array values;
    for (std::size_t i = 0, length = array.size(); i < length; ++i) values.push_back(number(array.number(double(i))));
    return values;
}
JsonValue attributeJSON(const std::shared_ptr<VertexAttribute>& attribute) {
    if (!attribute) return JsonValue{};
    return JsonValue::Object{{"count", number(attribute->getCount())},
        {"itemSize", number(attribute->getItemSize())}, {"normalized", attribute->getNormalized()},
        {"values", arrayJSON(attribute->getArray())}};
}
JsonValue geometryJSON(const BufferGeometry& geometry, const std::vector<std::string>& attributes) {
    JsonValue result = JsonValue::Object{};
    for (const auto& name : attributes) result[name] = attributeJSON(geometry.getAttribute(name));
    result["hasIndex"] = geometry.index != nullptr;
    result["index"] = geometry.index ? arrayJSON(geometry.index->getArray()) : JsonValue{};
    result["type"] = geometry.type;
    const JsonValue json = geometry.toJSON();
    JsonValue fields = JsonValue::Object{};
    for (const auto& name : {"slices", "stacks", "width", "height", "depth", "segments", "radius"})
        if (json.contains(name)) fields[name] = json.at(name);
    fields["type"] = json.at("type");
    result["json"] = std::move(fields);
    return result;
}
ParametricGeometry::Func parametricFunction(int id) {
    switch (id) {
        case 1: return ParametricFunctions::plane;
        case 2: return ParametricFunctions::klein;
        case 3: return ParametricFunctions::mobius;
        case 4: return ParametricFunctions::mobius3d;
        default: return {};
    }
}
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            JsonValue output = JsonValue::Object{};
            if (kind == "F") {
                int id = 0;
                stream >> id >> token;
                const double u = parse(token);
                stream >> token;
                const double v = parse(token);
                Vector3 target;
                switch (id) {
                    case 1: ParametricFunctions::plane(u, v, target); break;
                    case 2: ParametricFunctions::klein(u, v, target); break;
                    case 3: ParametricFunctions::mobius(u, v, target); break;
                    case 4: ParametricFunctions::mobius3d(u, v, target); break;
                    default: throw std::runtime_error("unknown function id");
                }
                output["point"] = JsonValue::Array{number(target.x), number(target.y), number(target.z)};
            } else if (kind == "P") {
                int id = 0;
                double slices = 0, stacks = 0;
                stream >> id >> slices >> stacks;
                ParametricGeometry geometry(parametricFunction(id), slices, stacks);
                output = geometryJSON(geometry, {"position", "normal", "uv"});
            } else if (kind == "B") {
                double width = 0, height = 0, depth = 0, ws = 0, hs = 0, ds = 0;
                {
                    double* targets[] = {&width, &height, &depth, &ws, &hs, &ds};
                    for (auto* target : targets) {
                        stream >> token;
                        *target = parse(token);
                    }
                }
                BoxLineGeometry geometry(width, height, depth, ws, hs, ds);
                output = geometryJSON(geometry, {"position"});
            } else if (kind == "R") {
                double width = 0, height = 0, depth = 0, segments = 0, radius = 0;
                {
                    // Tokens go through parse() so "NaN", "Infinity" and "-0" survive; the MSVC
                    // stream extractor does not accept those spellings and would leave zeros.
                    double* targets[] = {&width, &height, &depth, &segments, &radius};
                    for (auto* target : targets) {
                        stream >> token;
                        *target = parse(token);
                    }
                }
                RoundedBoxGeometry geometry(width, height, depth, segments, radius);
                output = geometryJSON(geometry, {"position", "normal", "uv"});
            } else {
                throw std::runtime_error("unknown scenario kind: " + kind);
            }
            std::cout << output.dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
