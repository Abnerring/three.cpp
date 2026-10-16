#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/ConvexGeometry.h"
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/ConvexGeometry.js under the pinned Node oracle.
namespace {

std::vector<Vector3> cubeCorners() {
    std::vector<Vector3> points;
    for (const double x : {-1., 1.})
        for (const double y : {-1., 1.})
            for (const double z : {-1., 1.}) points.push_back(Vector3(x, y, z));
    return points;
}
std::vector<Vector3> tetrahedron() {
    return {Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)};
}
// The pinned constructor stores the generated values in a Float32Array; the port uses
// Float32BufferAttribute, so every component is read back as the binary64 conversion of the
// stored float32 value.
const Float32Array& attributeArray(const ConvexGeometry& geometry, const std::string& name) {
    const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name));
    REQUIRE(attribute != nullptr);
    return std::get<Float32Array>(attribute->array);
}
std::vector<double> components(const ConvexGeometry& geometry, const std::string& name) {
    std::vector<double> values;
    for (const float value : attributeArray(geometry, name)) values.push_back(double(value));
    return values;
}

} // namespace

TEST_CASE("ConvexGeometry defaults and empty point clouds", "[convexgeometry]") {
    const ConvexGeometry geometry;

    SECTION("the pinned constructor defaults to an empty point array") {
        REQUIRE(geometry.type == "BufferGeometry");
        REQUIRE(geometry.index == nullptr);
        REQUIRE(geometry.hasAttribute("position"));
        REQUIRE(geometry.hasAttribute("normal"));
        REQUIRE_FALSE(geometry.hasAttribute("uv"));
        REQUIRE(geometry.getAttribute("position")->getItemSize() == 3.);
        REQUIRE(geometry.getAttribute("normal")->getItemSize() == 3.);
        REQUIRE(geometry.getAttribute("position")->getCount() == 0.);
        REQUIRE(geometry.getAttribute("normal")->getCount() == 0.);
        REQUIRE(components(geometry, "position").empty());
        REQUIRE(components(geometry, "normal").empty());
    }

    SECTION("fewer than four points produce no faces and therefore no vertices") {
        const std::vector<std::vector<Vector3>> clouds = {
            {},
            {Vector3(1, 2, 3)},
            {Vector3(0, 0, 0), Vector3(1, 1, 1)},
            {Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0)}};
        for (const std::vector<Vector3>& cloud : clouds) {
            const ConvexGeometry small(cloud);
            REQUIRE(small.getAttribute("position")->getCount() == 0.);
            REQUIRE(small.getAttribute("normal")->getCount() == 0.);
            REQUIRE(small.index == nullptr);
        }
    }
}

TEST_CASE("ConvexGeometry builds the pinned tetrahedron", "[convexgeometry]") {
    const ConvexGeometry geometry(tetrahedron());

    SECTION("type, sizes and the missing index of the pinned class") {
        REQUIRE(geometry.type == "BufferGeometry");
        REQUIRE(geometry.index == nullptr);
        REQUIRE(geometry.getAttribute("position")->getCount() == 12.);
        REQUIRE(geometry.getAttribute("normal")->getCount() == 12.);
        REQUIRE(geometry.getAttribute("position")->getItemSize() == 3.);
        REQUIRE_FALSE(geometry.getAttribute("position")->getNormalized());
        REQUIRE_FALSE(geometry.hasAttribute("uv"));
        REQUIRE(std::holds_alternative<Float32Array>(
            std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute("position"))->array));
    }

    SECTION("the position array of the pinned reference run") {
        const std::vector<double> expected = {0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
                                              0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0};
        REQUIRE(components(geometry, "position") == expected);
    }

    SECTION("the normal array of the pinned reference run") {
        // The fourth face normal is 1 / sqrt( 3 ) after the float32 conversion of the pinned
        // Float32Array.
        const double diagonal = 0.5773502588272095;
        const std::vector<double> expected = {0, 0, -1, 0, 0, -1, 0, 0, -1,
                                              0, -1, 0, 0, -1, 0, 0, -1, 0,
                                              diagonal, diagonal, diagonal,
                                              diagonal, diagonal, diagonal,
                                              diagonal, diagonal, diagonal,
                                              -1, 0, 0, -1, 0, 0, -1, 0, 0};
        REQUIRE(components(geometry, "normal") == expected);
    }
}

TEST_CASE("ConvexGeometry builds the cube hull", "[convexgeometry]") {
    const ConvexGeometry geometry(cubeCorners());
    const std::vector<double> positions = components(geometry, "position");
    const std::vector<double> normals = components(geometry, "normal");

    SECTION("twelve triangles with twelve float32 positions each") {
        REQUIRE(geometry.getAttribute("position")->getCount() == 36.);
        REQUIRE(geometry.getAttribute("normal")->getCount() == 36.);
        REQUIRE(geometry.index == nullptr);
    }

    SECTION("all coordinates are cube corners and all normals are unit axis vectors") {
        REQUIRE(positions.size() == 108);
        for (const double value : positions) REQUIRE(std::fabs(value) == 1.);
        REQUIRE(normals.size() == 108);
        for (std::size_t i = 0; i < normals.size(); i += 3) {
            const double x = normals[i], y = normals[i + 1], z = normals[i + 2];
            REQUIRE(std::fabs(x) + std::fabs(y) + std::fabs(z) == 1.);
            REQUIRE(x * x + y * y + z * z == 1.);
        }
    }

    SECTION("every triangle is wound counter-clockwise around its stored normal") {
        for (std::size_t i = 0; i < positions.size(); i += 9) {
            const Vector3 a(positions[i + 0], positions[i + 1], positions[i + 2]);
            const Vector3 b(positions[i + 3], positions[i + 4], positions[i + 5]);
            const Vector3 c(positions[i + 6], positions[i + 7], positions[i + 8]);
            Vector3 normal;
            normal.subVectors(c, b).cross(Vector3().subVectors(a, b)).normalize();
            REQUIRE(std::fabs(normal.x - normals[i + 0]) < 1e-6);
            REQUIRE(std::fabs(normal.y - normals[i + 1]) < 1e-6);
            REQUIRE(std::fabs(normal.z - normals[i + 2]) < 1e-6);
        }
    }

    SECTION("a point cloud with interior points produces the same triangles") {
        std::vector<Vector3> withInterior = cubeCorners();
        withInterior.push_back(Vector3(0, 0, 0));
        withInterior.push_back(Vector3(0.25, 0.25, -0.25));
        withInterior.push_back(Vector3(-0.5, 0.5, 0.5));
        const ConvexGeometry inner(withInterior);
        REQUIRE(components(inner, "position") == positions);
        REQUIRE(components(inner, "normal") == normals);
    }
}

TEST_CASE("ConvexGeometry stores float32 rounded point coordinates", "[convexgeometry]") {
    const std::vector<Vector3> cloud = {Vector3(0, 0, 0), Vector3(0.1, 0, 0), Vector3(0, 0.1, 0),
                                        Vector3(0, 0, 0.1)};
    const ConvexGeometry geometry(cloud);
    const std::vector<double> positions = components(geometry, "position");
    REQUIRE(geometry.getAttribute("position")->getCount() == 12.);

    const double rounded = double(float(0.1));
    REQUIRE(rounded == 0.10000000149011612);
    std::size_t roundedComponents = 0;
    for (const double value : positions) {
        if (value == 0.) continue;
        REQUIRE(value == rounded);
        roundedComponents++;
    }
    // Every triangle uses three of the four corners, so each non-zero coordinate appears once
    // per triangle it belongs to.
    REQUIRE(roundedComponents == 9);
}

TEST_CASE("ConvexGeometry keeps the pinned clone and JSON behaviour", "[convexgeometry]") {
    const ConvexGeometry geometry(tetrahedron());

    SECTION("clone() is the inherited BufferGeometry clone, as upstream does not override it") {
        const std::shared_ptr<BufferGeometry> clone = geometry.clone();
        REQUIRE(clone != nullptr);
        REQUIRE(dynamic_cast<ConvexGeometry*>(clone.get()) == nullptr);
        REQUIRE(clone->getAttribute("position")->getCount() == 12.);
        REQUIRE(clone->type == "BufferGeometry");
    }

    SECTION("the class never assigns a type, so the JSON keeps the BufferGeometry type") {
        REQUIRE(geometry.toJSON().at("type").get<std::string>() == "BufferGeometry");
    }
}
