#include "geometries/PolyhedronGeometry.h"
#include "geometries/TetrahedronGeometry.h"
#include "geometries/OctahedronGeometry.h"
#include "geometries/IcosahedronGeometry.h"
#include "geometries/DodecahedronGeometry.h"
#include "geometries/ShapeGeometry.h"
#include "geometries/TubeGeometry.h"
#include "geometries/EdgesGeometry.h"
#include "geometries/WireframeGeometry.h"
#include "geometries/BoxGeometry.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/CatmullRomCurve3.h"
#include <iostream>

namespace {
JsonValue normalized(const JsonValue& value) {
    return std::visit([](const auto& item) -> JsonValue {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(item)) return "NaN";
            if (std::isinf(item)) return item < 0 ? "-Infinity" : "Infinity";
            if (item == 0 && std::signbit(item)) return "-0";
            return item;
        } else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            JsonValue::Array result; for (const auto& entry : item) result.push_back(normalized(entry)); return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            JsonValue::Object result;
            for (const auto& [key, entry] : item) result.emplace(key, key == "uuid" ? JsonValue("uuid") : normalized(entry));
            return result;
        } else return item;
    }, value.storage());
}
JsonValue points(const std::vector<Vector3>& vectors) {
    JsonValue::Array result;
    for (const auto& v : vectors) result.emplace_back(JsonValue::Array{v.x, v.y, v.z});
    return result;
}
JsonValue snapshot(const BufferGeometry& g) {
    JsonValue attributes = JsonValue::Object{};
    for (const auto& [name, attribute] : g.attributes) attributes[name] = attribute->toJSON();
    JsonValue::Array groups;
    for (const auto& group : g.groups) groups.emplace_back(JsonValue::Array{group.start, group.count, group.materialIndex});
    JsonValue result = JsonValue::Object{{"type", g.type}, {"attributes", attributes},
        {"index", g.index ? g.index->toJSON() : JsonValue(nullptr)}, {"groups", groups},
        {"json", g.toJSON().copyJSON()}};
    if (const auto tube = dynamic_cast<const TubeGeometry*>(&g)) {
        result["frames"] = JsonValue::Array{points(tube->tangents), points(tube->normals), points(tube->binormals)};
    }
    return result;
}
template <class Geometry> JsonValue polyhedra() {
    JsonValue::Array result;
    for (const auto [radius, detail] : {std::pair{1.,0.}, {2.5,1.}, {0.,2.}, {-2.,0.}, {1.,-1.}, {1.,-.5}, {1.,NAN}}) {
        Geometry geometry(radius, detail);
        result.push_back(snapshot(geometry));
    }
    Geometry geometry(1.3, 2);
    result.push_back(snapshot(*geometry.clone()));
    result.push_back(snapshot(*Geometry::fromJSON(geometry.toJSON())));
    geometry.translate(1, -2, 3); result.push_back(snapshot(geometry));
    return result;
}
std::shared_ptr<Shape> makeShape(bool hole, double offset = 0) {
    auto shape = std::make_shared<Shape>(); shape->uuid = offset == 0 ? "shape0" : "shape1";
    shape->moveTo(offset, 0).lineTo(offset + 8, 0).lineTo(offset + 8, 8).lineTo(offset + 3, 8)
        .quadraticCurveTo(offset - 1, 4, offset, 0);
    if (hole) {
        auto path = std::make_shared<Path>(); path->moveTo(offset + 2, 2).lineTo(offset + 2, 4)
            .lineTo(offset + 4, 4).lineTo(offset + 4, 2).lineTo(offset + 2, 2);
        shape->holes->push_back(path);
    }
    return shape;
}
JsonValue shapes() {
    JsonValue::Array result;
    ShapeGeometry defaultGeometry;
    std::get<std::shared_ptr<Shape>>(defaultGeometry.parameters.shapes)->uuid = "default";
    result.push_back(snapshot(defaultGeometry));
    auto a = makeShape(true), b = makeShape(false, 20);
    auto list = std::make_shared<ShapeGeometry::ShapeList>(ShapeGeometry::ShapeList{a, b});
    ShapeGeometry single(a, 4), array(list, 4), empty(std::make_shared<ShapeGeometry::ShapeList>(), 4);
    result.push_back(snapshot(single)); result.push_back(snapshot(array)); result.push_back(snapshot(empty));
    result.push_back(snapshot(*array.clone()));
    result.push_back(snapshot(*ShapeGeometry::fromJSON(single.toJSON(), {{a->uuid, a}})));
    const auto clone = std::dynamic_pointer_cast<ShapeGeometry>(array.clone());
    list->push_back(a);
    result.emplace_back(JsonValue::Object{{"arrayIdentity", std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(clone->parameters.shapes) == list},
        {"singleIdentity", std::get<std::shared_ptr<Shape>>(single.parameters.shapes) == a},
        {"changedParameters", array.toJSON().copyJSON()}, {"unchangedVertexCount", array.getAttribute("position")->getCount()}});
    single.translate(1, 2, 3); result.push_back(snapshot(single));
    return result;
}
JsonValue tubes() {
    JsonValue::Array result;
    result.push_back(snapshot(TubeGeometry()));
    auto line = std::make_shared<LineCurve3>(std::make_shared<Vector3>(1, -2, 3), std::make_shared<Vector3>(4, 5, -1));
    auto controls = std::make_shared<CatmullRomCurve3::Points>();
    for (const auto& v : {Vector3(0,0,0), Vector3(2,3,1), Vector3(-1,4,3), Vector3(-2,0,-1)}) controls->push_back(std::make_shared<Vector3>(v));
    auto curved = std::make_shared<CatmullRomCurve3>(controls, true);
    for (const bool closed : {false, true}) {
        result.push_back(snapshot(TubeGeometry(line, 3, .7, 5, closed)));
        result.push_back(snapshot(TubeGeometry(curved, 7, -.4, 4, closed)));
    }
    for (const auto radial : {0., 2.5, -1., double(NAN)}) result.push_back(snapshot(TubeGeometry(line, 2, 1, radial)));
    result.push_back(snapshot(TubeGeometry(line, 0, 1, 3)));
    TubeGeometry original(line, 3, .5, 4);
    auto clone = std::dynamic_pointer_cast<TubeGeometry>(original.clone());
    result.push_back(snapshot(*clone));
    result.emplace_back(JsonValue::Array{clone->parameters.path == line, clone->tangents.size(),
        clone->getAttribute("position") != original.getAttribute("position")});
    result.push_back(snapshot(*TubeGeometry::fromJSON(original.toJSON())));
    original.scale(2, 1, .5); result.push_back(snapshot(original));
    for (const auto segments : {.5, 1.5, 2.5}) {
        result.push_back(snapshot(TubeGeometry(curved, segments, 1, -1)));
        result.push_back(snapshot(TubeGeometry(curved, segments, 1, NAN)));
        const auto frames = curved->computeFrenetFrames(segments);
        result.emplace_back(JsonValue::Array{points(frames.tangents), points(frames.normals), points(frames.binormals)});
    }
    for (const auto segments : {-1., double(NAN), -double(INFINITY), .5, 2.5}) {
        for (const bool closed : {false, true}) {
            try {
                TubeGeometry invalid(line, segments, 1, 0, closed);
                result.emplace_back("success");
            } catch (const JSTypeError&) { result.emplace_back("TypeError"); }
            try {
                const auto frames = curved->computeFrenetFrames(segments, closed);
                result.emplace_back(JsonValue::Array{points(frames.tangents), points(frames.normals), points(frames.binormals)});
            } catch (const JSTypeError&) { result.emplace_back("TypeError"); }
        }
    }
    return result;
}
JsonValue edges() {
    JsonValue::Array result;
    result.push_back(snapshot(EdgesGeometry())); result.push_back(snapshot(WireframeGeometry()));
    auto box = std::make_shared<BoxGeometry>();
    for (const auto threshold : {0., 1., 90., 91., 180., -20., double(NAN)}) result.push_back(snapshot(EdgesGeometry(box, threshold)));
    result.push_back(snapshot(WireframeGeometry(box)));
    result.push_back(snapshot(EdgesGeometry(box->toNonIndexed())));
    result.push_back(snapshot(WireframeGeometry(box->toNonIndexed())));
    box->groups.clear(); box->addGroup(6, 6, 0); box->addGroup(0, 3, 1); box->addGroup(0, 3, 2);
    box->setDrawRange(0, 0);
    result.push_back(snapshot(WireframeGeometry(box))); result.push_back(snapshot(EdgesGeometry(box)));
    for (const auto& values : std::vector<std::vector<double>>{
        {0,0,0, 1,0,0, 0,1,0, 0,1,0, 1,0,0, 1,1,0},
        {0,0,0, .000049,0,0, 0,1,0},
        {0,0,0, .00005,0,0, 0,1,0},
        {0,0,0, 1,0,0, 2,0,0},
        {0,0,0, 1,0,0, NAN,1,0},
        {0,0,0, 1,0,0}, {}}) {
        auto geometry = std::make_shared<BufferGeometry>();
        geometry->setAttribute("position", std::make_shared<Float64BufferAttribute>(values, 3));
        result.push_back(snapshot(EdgesGeometry(geometry))); result.push_back(snapshot(WireframeGeometry(geometry)));
    }
    EdgesGeometry original(box); auto copied = std::dynamic_pointer_cast<EdgesGeometry>(original.clone());
    result.emplace_back(JsonValue::Array{copied->parameters.geometry == box,
        copied->getAttribute("position") != original.getAttribute("position")});
    result.push_back(snapshot(*copied)); original.translate(1, 2, 3); result.push_back(snapshot(original));
    return result;
}
}
int main() {
    try {
        JsonValue result = JsonValue::Object{{"tetrahedron", polyhedra<TetrahedronGeometry>()},
            {"octahedron", polyhedra<OctahedronGeometry>()}, {"icosahedron", polyhedra<IcosahedronGeometry>()},
            {"dodecahedron", polyhedra<DodecahedronGeometry>()}};
        auto vertices = std::make_shared<PolyhedronGeometry::Numbers>(PolyhedronGeometry::Numbers{1,0,0, 0,1,0, 0,0,1});
        auto indices = std::make_shared<PolyhedronGeometry::Numbers>(PolyhedronGeometry::Numbers{0,1,2});
        PolyhedronGeometry poly(vertices, indices, 1.7, 1);
        auto clone = std::dynamic_pointer_cast<PolyhedronGeometry>(poly.clone());
        result["polyhedron"] = JsonValue::Array{snapshot(PolyhedronGeometry()), snapshot(poly), snapshot(*PolyhedronGeometry::fromJSON(poly.toJSON())),
            JsonValue::Array{clone->parameters.vertices == vertices, clone->parameters.indices == indices,
                clone->getAttribute("position") != poly.getAttribute("position")}};
        result["shapes"] = shapes();
        result["tubes"] = tubes();
        result["edges"] = edges();
        JsonValue::Array earlySerialization;
        for (const auto& geometry : std::vector<std::shared_ptr<BufferGeometry>>{
            std::make_shared<PolyhedronGeometry>(), std::make_shared<TetrahedronGeometry>(),
            std::make_shared<OctahedronGeometry>(), std::make_shared<IcosahedronGeometry>(),
            std::make_shared<DodecahedronGeometry>(), std::make_shared<ShapeGeometry>(),
            std::make_shared<TubeGeometry>(), std::make_shared<EdgesGeometry>(),
            std::make_shared<WireframeGeometry>()}) {
            if (const auto shape = std::dynamic_pointer_cast<ShapeGeometry>(geometry))
                std::get<std::shared_ptr<Shape>>(shape->parameters.shapes)->uuid = "default";
            geometry->setAttribute("position", nullptr);
            geometry->morphAttributes["position"].push_back(nullptr);
            earlySerialization.push_back(geometry->toJSON());
        }
        result["earlySerialization"] = std::move(earlySerialization);
        std::cout << normalized(result).dump() << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
