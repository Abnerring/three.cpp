#include "catch.hpp"
#include "geometries/CircleGeometry.h"
#include "geometries/RingGeometry.h"
#include "geometries/SphereGeometry.h"
#include "geometries/CylinderGeometry.h"
#include "geometries/ConeGeometry.h"
#include "geometries/TorusGeometry.h"
#include "geometries/TorusKnotGeometry.h"
#include "geometries/LatheGeometry.h"
#include "geometries/CapsuleGeometry.h"
#include <limits>

TEST_CASE("Primitive geometries preserve raw parameters and upstream segment rounding", "[geometries]") {
    CircleGeometry circle(1, 3.5);
    REQUIRE(circle.parameters.segments == 3.5);
    REQUIRE(circle.getAttribute("position")->getCount() == 5);
    REQUIRE(circle.index->count == 9);
    RingGeometry ring(.5, 1, 3.5, 1.5);
    REQUIRE(ring.getAttribute("position")->getCount() == 8);
    REQUIRE(ring.index->count == 48);
    REQUIRE(ring.index->getX(3) == 4);
    SphereGeometry sphere(1, 3.9, 2.9);
    REQUIRE(sphere.parameters.widthSegments == 3.9);
    REQUIRE(sphere.getAttribute("position")->getCount() == 12);
    REQUIRE(sphere.index->count == 18);
    CapsuleGeometry capsule(1, -2, .5, 2, -3);
    REQUIRE(capsule.parameters.height == -2);
    REQUIRE(capsule.getAttribute("position")->getCount() == 16);
    REQUIRE(capsule.index->count == 54);
}
TEST_CASE("Cylinder caps and cone groups keep exact material partition order", "[geometries]") {
    CylinderGeometry cylinder(1, 2, 3, 4, 2);
    REQUIRE(cylinder.groups.size() == 3);
    REQUIRE(cylinder.groups[0].start == 0);
    REQUIRE(cylinder.groups[0].count == 48);
    REQUIRE(cylinder.groups[1].start == 48);
    REQUIRE(cylinder.groups[1].count == 12);
    REQUIRE(cylinder.groups[1].materialIndex == 1);
    REQUIRE(cylinder.groups[2].start == 60);
    REQUIRE(cylinder.groups[2].materialIndex == 2);
    REQUIRE(cylinder.getAttribute("position")->getCount() == 33);
    ConeGeometry cone(2, 3, 4, 2);
    REQUIRE(cone.groups.size() == 2);
    REQUIRE(cone.groups[0].count == 36);
    REQUIRE(cone.groups[1].materialIndex == 2);
    REQUIRE(dynamic_cast<CylinderGeometry*>(&cone) != nullptr);
    auto clone = std::dynamic_pointer_cast<ConeGeometry>(cone.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->parameters.radius == 2);
    REQUIRE(clone->toJSON().at("radius").get<double>() == 2);
    REQUIRE_FALSE(clone->toJSON().contains("radiusTop"));
}
TEST_CASE("Lathe parameters shallow-copy profile identity while geometry storage clones", "[geometries]") {
    auto points = std::make_shared<LatheGeometry::Points>(LatheGeometry::Points{
        std::make_shared<Vector2>(1, 0), std::make_shared<Vector2>(3, 4)});
    LatheGeometry lathe(points, 3);
    REQUIRE(lathe.parameters.points == points);
    REQUIRE(lathe.getAttribute("normal")->getY(1) == -2);
    REQUIRE(lathe.getAttribute("normal")->getZ(1) == 4);
    auto clone = std::dynamic_pointer_cast<LatheGeometry>(lathe.clone());
    REQUIRE(clone->parameters.points == points);
    REQUIRE(clone->getAttribute("position") != lathe.getAttribute("position"));
    REQUIRE(clone->index != lathe.index);
    const auto originalPosition = lathe.getAttribute("position")->getZ(0);
    (*points)[0]->x = 7;
    REQUIRE(clone->parameters.points->at(0)->x == 7);
    REQUIRE(lathe.getAttribute("position")->getZ(0) == originalPosition);
    REQUIRE(lathe.toJSON().at("points")[0].at("x").get<double>() == 7);
    auto restored = LatheGeometry::fromJSON(lathe.toJSON());
    REQUIRE(restored->parameters.points != points);
    REQUIRE(restored->getAttribute("position")->getZ(0) == 7);
    LatheGeometry empty(std::make_shared<LatheGeometry::Points>());
    REQUIRE(empty.getAttribute("position")->getCount() == 0);
    REQUIRE_THROWS_AS(LatheGeometry(std::make_shared<LatheGeometry::Points>(
        LatheGeometry::Points{std::make_shared<Vector2>()})), JSTypeError);
}
TEST_CASE("Primitive zero and NaN parameters follow terminating upstream cases", "[geometries]") {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    CircleGeometry circle(0, 3);
    REQUIRE(std::isnan(circle.getAttribute("uv")->getX(1)));
    TorusGeometry torus(1, .4, 0, 0);
    REQUIRE(torus.getAttribute("position")->getCount() == 1);
    REQUIRE(std::isnan(torus.getAttribute("position")->getX(0)));
    REQUIRE(torus.index->count == 0);
    SphereGeometry sphere(1, nan, 2);
    REQUIRE(sphere.getAttribute("position")->getCount() == 0);
    CapsuleGeometry capsule(0, 0, 1, 3, 1);
    REQUIRE(std::isnan(capsule.getAttribute("uv")->getY(0)));
    CylinderGeometry negative(1, 1, 1, -1, -1);
    REQUIRE(negative.getAttribute("position")->getCount() == 0);
    REQUIRE(negative.groups.size() == 3);
}
TEST_CASE("Primitive cloning and fromJSON preserve parameters and transformed serialization", "[geometries]") {
    std::vector<std::shared_ptr<BufferGeometry>> geometries{
        std::make_shared<CircleGeometry>(), std::make_shared<RingGeometry>(),
        std::make_shared<SphereGeometry>(), std::make_shared<CylinderGeometry>(),
        std::make_shared<ConeGeometry>(), std::make_shared<TorusGeometry>(),
        std::make_shared<TorusKnotGeometry>(), std::make_shared<LatheGeometry>(),
        std::make_shared<CapsuleGeometry>()};
    for (const auto& geometry : geometries) {
        INFO(geometry->type);
        auto clone = geometry->clone();
        REQUIRE(clone->type == geometry->type);
        REQUIRE(clone->uuid != geometry->uuid);
        REQUIRE(clone->getAttribute("position") != geometry->getAttribute("position"));
        REQUIRE(clone->index != geometry->index);
        REQUIRE_FALSE(geometry->toJSON().contains("data"));
        geometry->translate(1, 2, 3);
        REQUIRE(geometry->toJSON().contains("data"));
        REQUIRE(geometry->toJSON().at("type").get<std::string>() == "BufferGeometry");
        REQUIRE(geometry->clone()->toJSON().at("type").get<std::string>() == "BufferGeometry");
    }
    CircleGeometry metadataOnly;
    metadataOnly.setAttribute("unused", nullptr);
    REQUIRE(metadataOnly.toJSON().at("radius").get<double>() == 1);
    auto defaults = TorusGeometry::fromJSON(JsonValue::Object{});
    REQUIRE(defaults->parameters.radialSegments == 12);
    REQUIRE(defaults->parameters.tubularSegments == 48);
    CircleGeometry circle(2, 6);
    auto clone = std::dynamic_pointer_cast<CircleGeometry>(circle.clone());
    circle.parameters.radius = 5;
    REQUIRE(clone->parameters.radius == 2);
    REQUIRE(CircleGeometry::fromJSON(circle.toJSON())->parameters.radius == 5);
}
TEST_CASE("Primitive indices switch to Uint32 at the upstream restart boundary", "[geometries]") {
    CircleGeometry circle(1, 65535);
    REQUIRE(circle.index->arrayType() == "Uint32Array");
    REQUIRE(circle.index->getX(circle.index->count - 2) == 65536);
}
