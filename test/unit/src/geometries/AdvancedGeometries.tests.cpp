#include "catch.hpp"
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

TEST_CASE("Polyhedron families preserve face counts and spherical subdivision", "[advanced-geometries]") {
    REQUIRE(TetrahedronGeometry().getAttribute("position")->getCount() == 12);
    REQUIRE(OctahedronGeometry().getAttribute("position")->getCount() == 24);
    REQUIRE(IcosahedronGeometry().getAttribute("position")->getCount() == 60);
    REQUIRE(DodecahedronGeometry().getAttribute("position")->getCount() == 108);
    IcosahedronGeometry geometry(2.5, 2);
    REQUIRE_FALSE(geometry.index);
    REQUIRE(geometry.getAttribute("position")->getCount() == 60 * 9);
    Vector3 vertex;
    for (double i = 0; i < geometry.getAttribute("position")->getCount(); ++i) {
        vertex.fromBufferAttribute(*geometry.getAttribute("position"), i);
        REQUIRE(vertex.length() == Approx(2.5).margin(1e-6));
        vertex.fromBufferAttribute(*geometry.getAttribute("normal"), i);
        REQUIRE(vertex.length() == Approx(1).margin(1e-6));
    }
}
TEST_CASE("Polyhedron parameter arrays retain identity while buffers remain independent", "[advanced-geometries]") {
    auto vertices = std::make_shared<PolyhedronGeometry::Numbers>(PolyhedronGeometry::Numbers{1,0,0, 0,1,0, 0,0,1});
    auto indices = std::make_shared<PolyhedronGeometry::Numbers>(PolyhedronGeometry::Numbers{0,1,2});
    PolyhedronGeometry geometry(vertices, indices, 2, 0);
    auto clone = std::dynamic_pointer_cast<PolyhedronGeometry>(geometry.clone());
    REQUIRE(clone->parameters.vertices == vertices);
    REQUIRE(clone->parameters.indices == indices);
    REQUIRE(clone->getAttribute("position") != geometry.getAttribute("position"));
    const auto original = geometry.getAttribute("position")->toJSON();
    vertices->at(0) = 4;
    REQUIRE(geometry.getAttribute("position")->toJSON() == original);
    REQUIRE(geometry.toJSON().at("vertices")[0].get<double>() == 4);
    auto restored = PolyhedronGeometry::fromJSON(geometry.toJSON());
    REQUIRE(restored->parameters.vertices->at(0) == 4);
    REQUIRE(PolyhedronGeometry().getAttribute("position")->getCount() == 0);
    REQUIRE(TetrahedronGeometry(1, -1).getAttribute("position")->getCount() == 0);
}
TEST_CASE("Polyhedron cloning preserves subclass and transformed serialization", "[advanced-geometries]") {
    DodecahedronGeometry geometry(3, 1);
    REQUIRE(std::dynamic_pointer_cast<DodecahedronGeometry>(geometry.clone()));
    REQUIRE_FALSE(geometry.toJSON().contains("vertices"));
    const auto restored = DodecahedronGeometry::fromJSON(geometry.toJSON());
    REQUIRE(restored->getAttribute("uv")->toJSON() == geometry.getAttribute("uv")->toJSON());
    geometry.translate(1, 2, 3);
    const auto json = geometry.toJSON();
    REQUIRE(json.at("type").get<std::string>() == "BufferGeometry");
    REQUIRE(json.contains("data"));
    REQUIRE_FALSE(json.contains("radius"));
}
TEST_CASE("ShapeGeometry keeps single and array groups distinct and triangulates holes", "[advanced-geometries]") {
    auto shape = std::make_shared<Shape>();
    shape->moveTo(0, 0).lineTo(10, 0).lineTo(10, 10).lineTo(0, 10).lineTo(0, 0);
    auto hole = std::make_shared<Path>();
    hole->moveTo(2, 2).lineTo(2, 6).lineTo(6, 6).lineTo(6, 2).lineTo(2, 2);
    shape->holes->push_back(hole);
    ShapeGeometry single(shape);
    REQUIRE(single.groups.empty());
    double area = 0;
    const auto position = single.getAttribute("position");
    for (double i = 0; i < single.index->count; i += 3) {
        Vector3 a, b, c;
        a.fromBufferAttribute(*position, single.index->getX(i));
        b.fromBufferAttribute(*position, single.index->getX(i + 1));
        c.fromBufferAttribute(*position, single.index->getX(i + 2));
        area += b.sub(a).cross(c.sub(a)).length() / 2;
    }
    REQUIRE(area == 84);
    auto shapes = std::make_shared<ShapeGeometry::ShapeList>(ShapeGeometry::ShapeList{shape});
    ShapeGeometry multiple(shapes);
    REQUIRE(multiple.groups.size() == 1);
    REQUIRE(multiple.groups[0].count == std::size_t(single.index->count));
    REQUIRE(multiple.index->toJSON() == single.index->toJSON());
    REQUIRE(std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(multiple.parameters.shapes) == shapes);
    auto clone = std::dynamic_pointer_cast<ShapeGeometry>(multiple.clone());
    REQUIRE(std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(clone->parameters.shapes) == shapes);
    ShapeGeometry::ShapeMap dictionary{{shape->uuid, shape}};
    auto restored = ShapeGeometry::fromJSON(single.toJSON(), dictionary);
    REQUIRE(restored->groups.size() == 1);
    REQUIRE(std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(restored->parameters.shapes)->at(0) == shape);
}
TEST_CASE("TubeGeometry creates rings and preserves shared path and upstream copy frames", "[advanced-geometries]") {
    auto path = std::make_shared<LineCurve3>(std::make_shared<Vector3>(), std::make_shared<Vector3>(0, 0, 4));
    TubeGeometry geometry(path, 2, .5, 4, false);
    REQUIRE(geometry.getAttribute("position")->getCount() == 15);
    REQUIRE(geometry.index->count == 48);
    REQUIRE(geometry.tangents.size() == 3);
    auto clone = std::dynamic_pointer_cast<TubeGeometry>(geometry.clone());
    REQUIRE(clone->parameters.path == path);
    REQUIRE(clone->tangents.size() == 65);
    REQUIRE(clone->getAttribute("position")->toJSON() == geometry.getAttribute("position")->toJSON());
    auto restored = TubeGeometry::fromJSON(geometry.toJSON());
    REQUIRE(restored->parameters.path != path);
    REQUIRE(restored->getAttribute("position")->toJSON() == geometry.getAttribute("position")->toJSON());
    TubeGeometry closed(path, 3, .5, 4, true);
    const auto positions = closed.getAttribute("position");
    for (int j = 0; j < 5; ++j) {
        REQUIRE(positions->getX(j) == positions->getX(15 + j));
        REQUIRE(positions->getY(j) == positions->getY(15 + j));
        REQUIRE(positions->getZ(j) == positions->getZ(15 + j));
    }
}
TEST_CASE("Frenet and Tube fractional subdivisions retain sampling and deferred reads", "[advanced-geometries]") {
    class SampledLine : public LineCurve3 {
      public:
        std::vector<double> tangentSamples, pointSamples;
        SampledLine() : LineCurve3(std::make_shared<Vector3>(), std::make_shared<Vector3>(0, 0, 1)) {}
        Vector3& getTangentAt(double u, Vector3& target) override {
            tangentSamples.push_back(u); return target.set(0, 0, 1);
        }
        Vector3& getPointAt(double u, Vector3& target) override {
            pointSamples.push_back(u); return target.set(0, 0, u);
        }
    };
    auto path = std::make_shared<SampledLine>();
    TubeGeometry geometry(path, 2.5, 1, -1);
    REQUIRE(geometry.parameters.tubularSegments == 2.5);
    REQUIRE(geometry.getAttribute("position")->getCount() == 0);
    REQUIRE(geometry.index->count == 0);
    REQUIRE(geometry.tangents.size() == 3);
    REQUIRE(path->tangentSamples == std::vector<double>{0, .4, .8});
    REQUIRE(path->pointSamples == std::vector<double>{0, .4, .8, 1});
    path->tangentSamples.clear(); path->pointSamples.clear();
    REQUIRE_THROWS_AS(TubeGeometry(path, 2.5, 1, 0), JSTypeError);
    REQUIRE(path->tangentSamples == std::vector<double>{0, .4, .8});
    REQUIRE(path->pointSamples == std::vector<double>{0, .4, .8, 1});
    path->pointSamples.clear();
    REQUIRE_THROWS_AS(TubeGeometry(path, 2.5, 1, -1, true), JSTypeError);
    REQUIRE(path->pointSamples.empty());
    for (const double segments : {-1., double(NAN), -double(INFINITY)})
        REQUIRE_THROWS_AS(path->computeFrenetFrames(segments), JSTypeError);
    REQUIRE_THROWS_AS(path->computeFrenetFrames(INFINITY), JSRangeError);
}
TEST_CASE("EdgesGeometry removes smooth diagonals while Wireframe retains them", "[advanced-geometries]") {
    auto box = std::make_shared<BoxGeometry>();
    EdgesGeometry edges(box);
    WireframeGeometry wire(box);
    REQUIRE(edges.getAttribute("position")->getCount() == 24);
    REQUIRE(wire.getAttribute("position")->getCount() == 36);
    REQUIRE(EdgesGeometry(box, 91).getAttribute("position")->getCount() == 0);
    REQUIRE(std::dynamic_pointer_cast<EdgesGeometry>(edges.clone())->parameters.geometry == box);
    REQUIRE(std::dynamic_pointer_cast<WireframeGeometry>(wire.clone())->parameters.geometry == box);
    REQUIRE_FALSE(EdgesGeometry().hasAttribute("position"));
    REQUIRE_FALSE(WireframeGeometry().hasAttribute("position"));
    box->groups.clear(); box->addGroup(0, 3, 0); box->setDrawRange(0, 0);
    REQUIRE(WireframeGeometry(box).getAttribute("position")->getCount() == 6);
    REQUIRE(EdgesGeometry(box).getAttribute("position")->getCount() == 24);
}
TEST_CASE("Advanced parameter serialization does not visit vertex storage", "[advanced-geometries]") {
    const std::vector<std::shared_ptr<BufferGeometry>> geometries{
        std::make_shared<PolyhedronGeometry>(), std::make_shared<TetrahedronGeometry>(),
        std::make_shared<OctahedronGeometry>(), std::make_shared<IcosahedronGeometry>(),
        std::make_shared<DodecahedronGeometry>(), std::make_shared<ShapeGeometry>(),
        std::make_shared<TubeGeometry>(), std::make_shared<EdgesGeometry>(),
        std::make_shared<WireframeGeometry>()};
    for (const auto& geometry : geometries) {
        const auto expected = geometry->toJSON();
        geometry->setAttribute("position", nullptr);
        geometry->morphAttributes["position"].push_back(nullptr);
        REQUIRE(geometry->toJSON() == expected);
        REQUIRE_FALSE(geometry->toJSON().contains("data"));
    }
}
