// Differential probe for the pinned examples/jsm/math/MeshSurfaceSampler.js port.
//
// Protocol, one scenario per stdin line:
//
//   M <meshKind> <weightAttribute|-> <sampleCount> <rng0> <rng1> ...
//
// meshKind: 0 BoxGeometry indexed, 1 BoxGeometry non-indexed, 2 BoxGeometry indexed with a
// colour attribute used as weight, 3 BoxGeometry non-indexed with a colour attribute,
// 4 SphereGeometry indexed, 5 an empty geometry (error path).
//
// The injected random source is an indexed pull with wraparound, implemented identically on
// both sides, so the sample sequence is reproducible. Every scenario dumps the float32
// distribution of the sampler plus position/normal/colour/uv of each sample.
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/JsonValue.h"
#include "geometries/BoxGeometry.h"
#include "geometries/SphereGeometry.h"
#include "math/Color.h"
#include "math/MeshSurfaceSampler.h"
#include "math/Vector2.h"
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
JsonValue vectorJSON(const Vector3& value) { return JsonValue::Array{number(value.x), number(value.y), number(value.z)}; }
JsonValue vectorJSON(const Vector2& value) { return JsonValue::Array{number(value.x), number(value.y)}; }
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
// The colour attribute both sides derive from the vertex index, exactly with integer maths.
void addColor(std::shared_ptr<BufferGeometry>& geometry) {
    const double count = geometry->getAttribute("position")->getCount();
    std::vector<double> values;
    for (double i = 0; i < count; i += 1) {
        values.push_back(std::fmod(i, 7.) / 7.);
        values.push_back(std::fmod(i * 3., 5.) / 5.);
        values.push_back(std::fmod(i * 5., 11.) / 11.);
    }
    geometry->setAttribute("color", std::make_shared<Float32BufferAttribute>(values, 3));
}
std::shared_ptr<Mesh> buildMesh(double kind) {
    if (kind == 5) return std::make_shared<Mesh>(); // empty geometry: no position attribute
    std::shared_ptr<BufferGeometry> geometry;
    if (kind == 0 || kind == 2) {
        geometry = std::make_shared<BoxGeometry>(1, 1, 1);
        if (kind == 2) addColor(geometry);
    } else if (kind == 1 || kind == 3) {
        geometry = std::make_shared<BoxGeometry>(1, 1, 1)->toNonIndexed();
        if (kind == 3) addColor(geometry);
    } else if (kind == 4) {
        geometry = std::make_shared<SphereGeometry>(1, 8, 6);
    } else {
        throw std::runtime_error("unknown mesh kind");
    }
    auto mesh = std::make_shared<Mesh>(geometry);
    mesh->updateMatrixWorld(true);
    return mesh;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token, weightName;
        stream >> kind;
        try {
            if (kind != "M") throw std::runtime_error("unknown scenario kind: " + kind);
            double meshKind = 0, sampleCount = 0;
            stream >> meshKind >> weightName >> sampleCount;
            std::vector<double> values;
            while (stream >> token) values.push_back(parse(token));
            const auto mesh = buildMesh(meshKind);
            MeshSurfaceSampler sampler(mesh);
            sampler.setWeightAttribute(weightName == "-" ? std::string() : weightName);
            sampler.build();
            std::size_t cursor = 0;
            sampler.setRandomGenerator([&values, &cursor]() {
                if (values.empty()) throw std::runtime_error("no random values provided");
                return values[cursor++ % values.size()];
            });
            JsonValue::Array distribution;
            for (const auto value : sampler.distribution) distribution.push_back(number(value));
            JsonValue::Array samples;
            for (double i = 0; i < sampleCount; i += 1) {
                Vector3 position, normal;
                Color color;
                Vector2 uv;
                sampler.sample(position, &normal, &color, &uv);
                samples.push_back(JsonValue::Object{{"position", vectorJSON(position)}, {"normal", vectorJSON(normal)},
                    {"color", JsonValue::Array{number(color.r), number(color.g), number(color.b)}},
                    {"uv", vectorJSON(uv)}});
            }
            std::cout << JsonValue(JsonValue::Object{{"distribution", std::move(distribution)},
                                      {"samples", std::move(samples)}})
                             .dump()
                      << '\n';
        } catch (const JSTypeError& error) {
            std::cout << JsonValue(JsonValue::Object{{"error", "TypeError"}, {"message", error.what()}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
