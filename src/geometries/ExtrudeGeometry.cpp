#include "geometries/ExtrudeGeometry.h"
#include "extras/ShapeUtils.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/CubicBezierCurve3.h"
#include "extras/curves/CatmullRomCurve3.h"
#include "math/MathUtils.h"
#include <numbers>

namespace {
using Points = Shape::Points;
JSValue at(const std::vector<double>& values, double index) {
    return index >= 0 && index < double(values.size()) && std::trunc(index) == index
        ? JSValue(values[static_cast<std::size_t>(index)]) : JSValue();
}
JSValue at(const JSValue& values, double index) { return values.get(JSValue(index).toString()); }
double coordinate(const JSValue& value) { return value.isUndefined() ? 0 : value.toNumber(); }
double subdivision(const JSValue& value) {
    const auto number = value.toNumber();
    if (number == INFINITY) throw JSRangeError("ExtrudeGeometry infinite subdivision does not terminate");
    return number;
}
void append(JSValue& array, const JSValue& value) { array.set(std::to_string(array.length()), value); }
const Vector2& point(const std::shared_ptr<Vector2>& value) {
    if (!value) throw JSTypeError("ExtrudeGeometry point is undefined");
    return *value;
}
double add(const JSValue& a, const JSValue& b) {
    const auto x = a.toPrimitive(), y = b.toPrimitive();
    if (x.type() == JSValue::Type::String || y.type() == JSValue::Type::String)
        return JSValue(x.toString() + y.toString()).toNumber();
    return x.toNumber() + y.toNumber();
}
JSValue option(const ExtrudeGeometry::OptionsPtr& options, const char* key, JSValue fallback) {
    if (!options) throw JSTypeError("ExtrudeGeometry options is null");
    return options->values.get(key).isUndefined() ? fallback : options->values.get(key);
}
void mergeOverlappingPoints(Points& points) {
    constexpr double threshold = 1e-10, thresholdSq = threshold * threshold;
    auto previous = points.empty() ? nullptr : points[0];
    for (std::size_t i = 1; i <= points.size(); ++i) {
        const auto index = i % points.size();
        const auto current = points[index];
        const auto& a = point(current); const auto& b = point(previous);
        const auto dx = a.x - b.x, dy = a.y - b.y, distance = dx * dx + dy * dy;
        auto scale = MathUtils::max(std::abs(a.x), std::abs(a.y));
        scale = MathUtils::max(scale, std::abs(b.x)); scale = MathUtils::max(scale, std::abs(b.y));
        if (distance <= thresholdSq * scale * scale) {
            points.erase(points.begin() + index); --i; continue;
        }
        previous = current;
    }
}
Vector2 bevelVector(const Vector2& p, const Vector2& previous, const Vector2& next) {
    double tx, ty, shrink;
    const auto px = p.x - previous.x, py = p.y - previous.y;
    const auto nx = next.x - p.x, ny = next.y - p.y;
    const auto lengthSq = px * px + py * py;
    const auto collinear = px * ny - py * nx;
    constexpr double epsilon = std::numeric_limits<double>::epsilon();
    if (std::abs(collinear) > epsilon) {
        const auto previousLength = std::sqrt(lengthSq), nextLength = std::sqrt(nx * nx + ny * ny);
        const auto ax = previous.x - py / previousLength, ay = previous.y + px / previousLength;
        const auto bx = next.x - ny / nextLength, by = next.y + nx / nextLength;
        const auto factor = ((bx - ax) * ny - (by - ay) * nx) / (px * ny - py * nx);
        tx = ax + px * factor - p.x; ty = ay + py * factor - p.y;
        const auto translationSq = tx * tx + ty * ty;
        if (translationSq <= 2) return Vector2(tx, ty);
        shrink = std::sqrt(translationSq / 2);
    } else {
        bool same = false;
        if (px > epsilon) { if (nx > epsilon) same = true; }
        else if (px < -epsilon) { if (nx < -epsilon) same = true; }
        else {
            const auto sign = [](double x) { return x == 0 || std::isnan(x) ? x : x > 0 ? 1. : -1.; };
            if (sign(py) == sign(ny)) same = true;
        }
        if (same) { tx = -py; ty = px; shrink = std::sqrt(lengthSq); }
        else { tx = px; ty = py; shrink = std::sqrt(lengthSq / 2); }
    }
    return Vector2(tx / shrink, ty / shrink);
}
std::vector<Vector2> movements(const Points& points) {
    std::vector<Vector2> result;
    for (std::size_t i = 0, count = points.size(); i < count; ++i)
        result.push_back(bevelVector(point(points[i]), point(points[(i + count - 1) % count]), point(points[(i + 1) % count])));
    return result;
}
Vector2 scalePoint(const std::shared_ptr<Vector2>& p, const Vector2& movement, double size) {
    auto result = point(p); return result.addScaledVector(movement, size);
}
class WorldUV final : public ExtrudeGeometry::UVGenerator {
  public:
    ExtrudeGeometry::UVs generateTopUV(ExtrudeGeometry&, ExtrudeGeometry::Vertices v, double a, double b, double c) override {
        std::vector<JSValue> coordinates;
        for (const auto index : {a, b, c}) {
            coordinates.push_back(at(v, index * 3)); coordinates.push_back(at(v, index * 3 + 1));
        }
        ExtrudeGeometry::UVs result;
        for (std::size_t i = 0; i < coordinates.size(); i += 2) {
            const auto x = coordinate(coordinates[i]), y = coordinate(coordinates[i + 1]);
            result.push_back(std::make_shared<Vector2>(x, y));
        }
        return result;
    }
    ExtrudeGeometry::UVs generateSideWallUV(ExtrudeGeometry&, ExtrudeGeometry::Vertices v, double a, double b, double c, double d) override {
        std::vector<JSValue> coordinates;
        for (const auto index : {a, b, c, d}) for (int component = 0; component < 3; ++component)
            coordinates.push_back(at(v, index * 3 + component));
        const auto ay = coordinates[1].toNumber(), by = coordinates[4].toNumber();
        const auto ax = coordinates[0].toNumber(), bx = coordinates[3].toNumber();
        const bool useX = std::abs(ay - by) < std::abs(ax - bx);
        ExtrudeGeometry::UVs result;
        for (std::size_t i = 0; i < coordinates.size(); i += 3) {
            const auto x = coordinate(coordinates[i + (useX ? 0 : 1)]), y = 1 - coordinates[i + 2].toNumber();
            result.push_back(std::make_shared<Vector2>(x, y));
        }
        return result;
    }
};
std::shared_ptr<Curve<Vector3>> parsePath(const JsonValue& data) {
    const auto type = data.at("type").get<std::string>();
    std::shared_ptr<Curve<Vector3>> result;
    if (type == "LineCurve3") result = std::make_shared<LineCurve3>();
    else if (type == "QuadraticBezierCurve3") result = std::make_shared<QuadraticBezierCurve3>();
    else if (type == "CubicBezierCurve3") result = std::make_shared<CubicBezierCurve3>();
    else if (type == "CatmullRomCurve3") result = std::make_shared<CatmullRomCurve3>();
    else throw JSTypeError("ExtrudeGeometry.fromJSON requires a built-in spatial curve");
    result->fromJSON(data); return result;
}
}
ExtrudeGeometry::UVs ExtrudeGeometry::UVGenerator::generateTopUV(ExtrudeGeometry&, Vertices, double, double, double) {
    throw JSTypeError("ExtrudeGeometry UVGenerator.generateTopUV is not a function");
}
ExtrudeGeometry::UVs ExtrudeGeometry::UVGenerator::generateSideWallUV(ExtrudeGeometry&, Vertices, double, double, double, double) {
    throw JSTypeError("ExtrudeGeometry UVGenerator.generateSideWallUV is not a function");
}
std::shared_ptr<ExtrudeGeometry::UVGenerator> ExtrudeGeometry::worldUVGenerator() {
    static auto result = std::make_shared<WorldUV>(); return result;
}
std::shared_ptr<Shape> ExtrudeGeometry::defaultShape() {
    auto points = std::make_shared<Shape::Points>();
    for (const auto& p : {Vector2(.5, .5), Vector2(-.5, .5), Vector2(-.5, -.5), Vector2(.5, -.5)})
        points->push_back(std::make_shared<Vector2>(p));
    return std::make_shared<Shape>(points);
}
ExtrudeGeometry::ExtrudeGeometry(Shapes shapes, OptionsPtr options) : parameters{shapes, options} {
    type = "ExtrudeGeometry";
    auto output = JSValue::array();
    std::vector<double> uvOutput;
    const auto addShape = [&](const std::shared_ptr<Shape>& shape) {
        std::vector<double> placeholder;
        const auto curveSegments = option(options, "curveSegments", 12);
        const auto steps = option(options, "steps", 1);
        const auto depth = option(options, "depth", 1);
        auto bevelEnabled = option(options, "bevelEnabled", true).toBoolean();
        auto bevelThickness = option(options, "bevelThickness", .2);
        auto bevelSize = options->values.get("bevelSize").isUndefined()
            ? JSValue(bevelThickness.toNumber() - .1) : options->values.get("bevelSize");
        auto bevelOffset = option(options, "bevelOffset", 0);
        auto bevelSegments = option(options, "bevelSegments", 3);
        const auto pathValue = options->values.get("extrudePath");
        const auto path = options->extrudePath;
        const auto uvValue = option(options, "UVGenerator", JSValue());
        const auto uvgen = options->uvGenerator ? options->uvGenerator : uvValue.isUndefined() ? worldUVGenerator() : nullptr;
        const bool byPath = bool(path) || pathValue.toBoolean();
        Curve<Vector3>::Points pathPoints;
        Curve<Vector3>::FrenetFrames frames;
        if (byPath) {
            if (!path) throw JSTypeError("ExtrudeGeometry extrudePath.getSpacedPoints is not a function");
            pathPoints = path->getSpacedPoints(steps.toNumber());
            bevelEnabled = false;
            const auto catmull = std::dynamic_pointer_cast<CatmullRomCurve3>(path);
            frames = path->computeFrenetFrames(steps.toNumber(), catmull && catmull->closed);
        }
        if (!bevelEnabled) { bevelSegments = 0; bevelThickness = 0; bevelSize = 0; bevelOffset = 0; }
        if (!shape) throw JSTypeError("ExtrudeGeometry shape is undefined");
        auto extracted = shape->extractPoints(curveSegments.toNumber());
        auto contour = std::move(extracted.shape);
        auto holes = std::move(extracted.holes);
        if (!ShapeUtils::isClockWise(contour)) {
            std::reverse(contour.begin(), contour.end());
            for (auto& hole : holes) if (ShapeUtils::isClockWise(hole)) std::reverse(hole.begin(), hole.end());
        }
        mergeOverlappingPoints(contour);
        for (auto& hole : holes) mergeOverlappingPoints(hole);
        auto vertices = contour;
        for (const auto& hole : holes) vertices.insert(vertices.end(), hole.begin(), hole.end());
        const auto count = vertices.size();
        const auto contourMovements = movements(contour);
        std::vector<std::vector<Vector2>> holeMovements;
        auto vertexMovements = contourMovements;
        for (const auto& hole : holes) {
            holeMovements.push_back(movements(hole));
            const auto& last = holeMovements.back(); vertexMovements.insert(vertexMovements.end(), last.begin(), last.end());
        }
        const auto push = [&](double x, double y, double z) { placeholder.insert(placeholder.end(), {x, y, z}); };
        ShapeUtils::Faces faces;
        if (bevelSegments.strictEquals(0)) faces = ShapeUtils::triangulateShape(contour, holes);
        else {
            Points contracted; std::vector<Points> expanded;
            for (double b = 0; b < subdivision(bevelSegments); ++b) {
                const auto t = b / bevelSegments.toNumber();
                const auto z = bevelThickness.toNumber() * std::cos(t * std::numbers::pi / 2);
                const auto bs = add(bevelSize.toNumber() * std::sin(t * std::numbers::pi / 2), bevelOffset);
                for (std::size_t i = 0; i < contour.size(); ++i) {
                    const auto v = scalePoint(contour[i], contourMovements[i], bs); push(v.x, v.y, -z);
                    if (t == 0) contracted.push_back(std::make_shared<Vector2>(v));
                }
                for (std::size_t h = 0; h < holes.size(); ++h) {
                    Points expandedHole;
                    for (std::size_t i = 0; i < holes[h].size(); ++i) {
                        const auto v = scalePoint(holes[h][i], holeMovements[h][i], bs); push(v.x, v.y, -z);
                        if (t == 0) expandedHole.push_back(std::make_shared<Vector2>(v));
                    }
                    if (t == 0) expanded.push_back(std::move(expandedHole));
                }
            }
            faces = ShapeUtils::triangulateShape(contracted, expanded);
        }
        const auto size = add(bevelSize, bevelOffset);
        const auto pushPath = [&](const Vector2& vertex, std::size_t segment) {
            if (segment >= frames.normals.size() || segment >= frames.binormals.size() ||
                segment >= pathPoints.size() || !pathPoints[segment]) throw JSTypeError("ExtrudeGeometry path frame is undefined");
            auto normal = frames.normals[segment], binormal = frames.binormals[segment];
            normal.multiplyScalar(vertex.x); binormal.multiplyScalar(vertex.y);
            auto position = *pathPoints[segment]; position.add(normal).add(binormal); push(position.x, position.y, position.z);
        };
        for (std::size_t i = 0; i < count; ++i) {
            const auto v = bevelEnabled ? scalePoint(vertices[i], vertexMovements[i], size) : point(vertices[i]);
            if (!byPath) push(v.x, v.y, 0); else pushPath(v, 0);
        }
        for (double s = 1; s <= subdivision(steps); ++s) {
            for (std::size_t i = 0; i < count; ++i) {
                const auto v = bevelEnabled ? scalePoint(vertices[i], vertexMovements[i], size) : point(vertices[i]);
                if (!byPath) push(v.x, v.y, depth.toNumber() / steps.toNumber() * s); else pushPath(v, static_cast<std::size_t>(s));
            }
        }
        for (double b = subdivision(bevelSegments) - 1; b >= 0; --b) {
            const auto t = b / bevelSegments.toNumber();
            const auto z = bevelThickness.toNumber() * std::cos(t * std::numbers::pi / 2);
            const auto bs = add(bevelSize.toNumber() * std::sin(t * std::numbers::pi / 2), bevelOffset);
            for (std::size_t i = 0; i < contour.size(); ++i) {
                const auto v = scalePoint(contour[i], contourMovements[i], bs); push(v.x, v.y, add(depth, z));
            }
            for (std::size_t h = 0; h < holes.size(); ++h) for (std::size_t i = 0; i < holes[h].size(); ++i) {
                const auto v = scalePoint(holes[h][i], holeMovements[h][i], bs); push(v.x, v.y, add(depth, z));
            }
        }
        const auto vertex = [&](double index) {
            for (int component = 0; component < 3; ++component) append(output, at(placeholder, index * 3 + component));
        };
        const auto uv = [&](const UVs& uvs, std::size_t index) {
            if (index >= uvs.size()) throw JSTypeError("ExtrudeGeometry UV is undefined");
            const auto& v = point(uvs[index]); uvOutput.insert(uvOutput.end(), {v.x, v.y});
        };
        const auto triangle = [&](double a, double b, double c) {
            vertex(a); vertex(b); vertex(c);
            const auto next = double(output.length()) / 3;
            if (!uvgen) throw JSTypeError("ExtrudeGeometry UVGenerator is null");
            const auto uvs = uvgen->generateTopUV(*this, output, next - 3, next - 2, next - 1);
            uv(uvs, 0); uv(uvs, 1); uv(uvs, 2);
        };
        const auto quad = [&](double a, double b, double c, double d) {
            vertex(a); vertex(b); vertex(d); vertex(b); vertex(c); vertex(d);
            const auto next = double(output.length()) / 3;
            if (!uvgen) throw JSTypeError("ExtrudeGeometry UVGenerator is null");
            const auto uvs = uvgen->generateSideWallUV(*this, output, next - 6, next - 3, next - 2, next - 1);
            for (const auto index : {0, 1, 3, 1, 2, 3}) uv(uvs, index);
        };
        auto start = double(output.length()) / 3;
        for (const auto& face : faces) triangle(double(face[2]), double(face[1]), double(face[0]));
        const auto layers = add(steps, bevelSegments.toNumber() * 2);
        const auto topOffset = count * (bevelEnabled ? layers : steps.toNumber());
        for (const auto& face : faces) triangle(face[0] + topOffset, face[1] + topOffset, face[2] + topOffset);
        addGroup(start, double(output.length()) / 3 - start, 0);
        start = double(output.length()) / 3;
        const auto sidewalls = [&](const Points& points, std::size_t offset) {
            for (std::size_t reverse = points.size(); reverse > 0; --reverse) {
                const auto j = reverse - 1, k = j == 0 ? points.size() - 1 : j - 1;
                for (double s = 0; s < layers; ++s) {
                    const auto first = count * s, second = count * (s + 1);
                    quad(offset + j + first, offset + k + first, offset + k + second, offset + j + second);
                }
            }
        };
        sidewalls(contour, 0);
        std::size_t offset = contour.size();
        for (const auto& hole : holes) { sidewalls(hole, offset); offset += hole.size(); }
        addGroup(start, double(output.length()) / 3 - start, 1);
    };
    if (const auto single = std::get_if<std::shared_ptr<Shape>>(&shapes)) addShape(*single);
    else {
        const auto list = std::get<std::shared_ptr<ShapeList>>(shapes);
        if (!list) throw JSTypeError("ExtrudeGeometry shapes is null");
        for (std::size_t i = 0, count = list->size(); i < count; ++i) {
            if (i >= list->size()) throw JSTypeError("ExtrudeGeometry shape is undefined");
            const auto shape = list->at(i); addShape(shape);
        }
    }
    std::vector<double> positions;
    for (std::uint32_t i = 0; i < output.length(); ++i) positions.push_back(output.get(std::to_string(i)).toNumber());
    setAttribute("position", std::make_shared<Float32BufferAttribute>(positions, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvOutput, 2));
    computeVertexNormals();
}
BufferGeometry& ExtrudeGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source); parameters = dynamic_cast<const ExtrudeGeometry&>(source).parameters; return *this;
}
std::shared_ptr<BufferGeometry> ExtrudeGeometry::clone() const {
    auto result = std::make_shared<ExtrudeGeometry>(); result->copy(*this); return result;
}
JsonValue ExtrudeGeometry::toJSON() const {
    JsonValue result;
    if (_transformed) { result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; }
    else {
        result = JsonValue::Object{{"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                {"generator", "BufferGeometry.toJSON"}}}, {"uuid", uuid}, {"type", type}, {"name", name}};
        if (userData.size()) result["userData"] = userData.sharedCopy();
        result["shapes"] = nullptr; result["options"] = nullptr;
    }
    const auto shapes = parameters.shapes;
    const auto options = parameters.options;
    JsonValue::Array ids;
    const auto pushShape = [&](const std::shared_ptr<Shape>& shape) {
        if (!shape) throw JSTypeError("ExtrudeGeometry shape is null"); ids.emplace_back(shape->uuid);
    };
    if (const auto single = std::get_if<std::shared_ptr<Shape>>(&shapes)) pushShape(*single);
    else {
        const auto list = std::get<std::shared_ptr<ShapeList>>(shapes);
        if (!list) throw JSTypeError("ExtrudeGeometry shapes is null");
        for (std::size_t i = 0, n = list->size(); i < n; ++i) pushShape(list->at(i));
    }
    result["shapes"] = std::move(ids);
    if (!options) throw JSTypeError("ExtrudeGeometry options is null");
    auto copiedOptions = JSValue::object();
    if (options->values.isObject()) {
        for (const auto& key : options->values.ownPropertyNames()) {
            const auto descriptor = options->values.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) copiedOptions.set(key, options->values.get(key));
        }
    } else if (options->values.type() == JSValue::Type::String) {
        for (double i = 0; i < options->values.get("length").toNumber(); ++i) {
            const auto key = JSValue(i).toString(); copiedOptions.set(key, options->values.get(key));
        }
    }
    auto serialized = copiedOptions.toJSON().value_or(JsonValue::Object{});
    if (!serialized.isObject()) serialized = JsonValue::Object{};
    if (options->uvGenerator) serialized["UVGenerator"] = options->uvGenerator->toJSON();
    const auto path = options->extrudePath;
    if (path) serialized["extrudePath"] = path->toJSON();
    else if (!options->values.get("extrudePath").isUndefined())
        throw JSTypeError("ExtrudeGeometry extrudePath.toJSON is not a function");
    result["options"] = std::move(serialized); return result;
}
std::shared_ptr<ExtrudeGeometry> ExtrudeGeometry::fromJSON(const JsonValue& data, const ShapeMap& dictionary) {
    auto shapes = std::make_shared<ShapeList>();
    for (const auto& id : data.at("shapes").get<JsonValue::Array>()) {
        const auto found = dictionary.find(id.get<std::string>());
        shapes->push_back(found == dictionary.end() ? nullptr : found->second);
    }
    const auto& serialized = data.at("options");
    auto options = std::make_shared<Options>(JSValue::fromJSON(serialized));
    if (serialized.contains("extrudePath")) options->setExtrudePath(parsePath(serialized.at("extrudePath")));
    if (serialized.contains("UVGenerator")) options->setUVGenerator(std::make_shared<UVGenerator>());
    return std::make_shared<ExtrudeGeometry>(shapes, options);
}
