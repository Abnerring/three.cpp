// Differential probe for the pinned addon geometries:
// examples/jsm/geometries/LoftGeometry.js and DecalGeometry.js.
//
// Protocol: one scenario per stdin line, tokens separated by spaces.
//
//   L <rows> <columns> <closed> <capStart> <capEnd> <x y z ...>
//       LoftGeometry from `rows` sections of `columns` points each.
//   D <px> <py> <pz> <ox> <oy> <oz> <sx> <sy> <sz>
//       DecalGeometry over a Mesh with BoxGeometry( 1, 1, 1 ) and an identity matrixWorld.
//   T <tx> <ty> <tz> <px> <py> <pz> <ox> <oy> <oz> <sx> <sy> <sz>
//       Same, but the mesh is translated by ( tx, ty, tz ) and its matrixWorld updated.
//   E <px> <py> <pz> <ox> <oy> <oz> <sx> <sy> <sz>
//       DecalGeometry over a non-indexed triangle soup without normals.
//
// Every geometry scenario dumps the attributes it defines (absent attributes stay null),
// the index, the type and the JSON parameter fields, so the comparison covers the structure
// and not only the numbers.
#include "core/BufferAttribute.h"
#include "core/JsonValue.h"
#include "geometries/BoxGeometry.h"
#include "geometries/DecalGeometry.h"
#include "geometries/LoftGeometry.h"
#include "math/Euler.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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
        {"itemSize", number(attribute->getItemSize())}, {"values", arrayJSON(attribute->getArray())}};
}
JsonValue geometryJSON(const BufferGeometry& geometry, const std::vector<std::string>& names) {
    JsonValue result = JsonValue::Object{};
    for (const auto& name : names) result[name] = attributeJSON(geometry.getAttribute(name));
    result["hasIndex"] = geometry.index != nullptr;
    result["index"] = geometry.index ? arrayJSON(geometry.index->getArray()) : JsonValue{};
    result["type"] = geometry.type;
    return result;
}
JsonValue loftJSON(const LoftGeometry& geometry) {
    JsonValue result = geometryJSON(geometry, {"position", "uv", "normal"});
    const JsonValue json = geometry.toJSON();
    result["json"] = JsonValue::Object{{"closed", json.at("closed")}, {"capStart", json.at("capStart")},
        {"capEnd", json.at("capEnd")}, {"sectionRows", double(json.at("sections").get<JsonValue::Array>().size())},
        {"type", json.at("type")}};
    return result;
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
std::shared_ptr<Mesh> triangleSoup() {
    // Two triangles in a plane, no normals, not indexed.
    std::vector<double> values{-0.75, -0.75, 0.25, 0.75, -0.75, 0.25, 0.75, 0.75, 0.25,
        -0.75, -0.75, 0.25, 0.75, 0.75, 0.25, -0.75, 0.75, 0.25};
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(values, 3));
    auto mesh = std::make_shared<Mesh>();
    mesh->geometry = geometry;
    mesh->updateMatrixWorld(true);
    return mesh;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            if (kind == "L") {
                double rows = 0, closed = 0, capStart = 0, capEnd = 0;
                stream >> rows >> closed >> capStart >> capEnd;
                std::vector<LoftGeometry::Section> sections;
                // Each row carries its own point count so ragged sections can be compared
                // against the module's "same number of points" guard.
                for (double i = 0; i < rows; i += 1) {
                    double columns = 0;
                    stream >> columns;
                    LoftGeometry::Section section;
                    for (double j = 0; j < columns; j += 1) {
                        stream >> token;
                        const double x = parse(token);
                        stream >> token;
                        const double y = parse(token);
                        stream >> token;
                        const double z = parse(token);
                        section.emplace_back(x, y, z);
                    }
                    sections.push_back(std::move(section));
                }
                LoftGeometry geometry(std::move(sections),
                    LoftGeometry::Options{closed != 0, capStart != 0, capEnd != 0});
                std::cout << loftJSON(geometry).dump() << '\n';
                continue;
            }
            if (kind == "D" || kind == "T" || kind == "E") {
                double tx = 0, ty = 0, tz = 0;
                if (kind == "T") stream >> tx >> ty >> tz;
                double px = 0, py = 0, pz = 0, ox = 0, oy = 0, oz = 0, sx = 0, sy = 0, sz = 0;
                stream >> px >> py >> pz >> ox >> oy >> oz >> sx >> sy >> sz;
                std::shared_ptr<Mesh> mesh;
                if (kind == "E") {
                    mesh = triangleSoup();
                } else {
                    mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1));
                }
                if (kind == "T") mesh->position.set(tx, ty, tz);
                mesh->updateMatrixWorld(true);
                DecalGeometry geometry(mesh, Vector3(px, py, pz), Euler(ox, oy, oz), Vector3(sx, sy, sz));
                std::cout << geometryJSON(geometry, {"position", "uv", "normal"}).dump() << '\n';
                continue;
            }
            throw std::runtime_error("unknown scenario kind: " + kind);
        } catch (const JSTypeError& error) {
            // The pinned module throws the same V8 TypeError for these inputs.
            std::cout << JsonValue(JsonValue::Object{{"error", "TypeError"}, {"message", error.what()}}).dump()
                      << '\n';
        } catch (const JSRangeError& error) {
            std::cout << JsonValue(JsonValue::Object{{"error", "RangeError"}, {"message", error.what()}}).dump()
                      << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
