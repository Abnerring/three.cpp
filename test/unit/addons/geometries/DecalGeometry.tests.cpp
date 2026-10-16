#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/BoxGeometry.h"
#include "geometries/DecalGeometry.h"
#include "math/Euler.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"
#include <cmath>
#include <memory>

// The reference values in this file come from the pinned examples/jsm/geometries/DecalGeometry.js
// (the differential probe compares the same scenarios bit for bit).
namespace {
std::shared_ptr<Mesh> boxMesh() {
    auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1));
    mesh->updateMatrixWorld(true);
    return mesh;
}
std::shared_ptr<Mesh> triangleSoup() {
    std::vector<double> values{-0.75, -0.75, 0.25, 0.75, -0.75, 0.25, 0.75, 0.75, 0.25,
        -0.75, -0.75, 0.25, 0.75, 0.75, 0.25, -0.75, 0.75, 0.25};
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(values, 3));
    auto mesh = std::make_shared<Mesh>(geometry);
    mesh->updateMatrixWorld(true);
    return mesh;
}
double component(const BufferGeometry& geometry, const std::string& name, std::size_t index) {
    return std::get<Float32Array>(
        std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name))->array)[index];
}
}

TEST_CASE("DecalGeometry clips a box into the projector volume", "[decalgeometry]") {
    DecalGeometry geometry(boxMesh(), Vector3(0, 0, 0), Euler(0, 0, 0), Vector3(1, 1, 1));
    REQUIRE(geometry.getAttribute("position")->getCount() == 36);
    REQUIRE(geometry.getAttribute("uv")->getCount() == 36);
    REQUIRE(geometry.getAttribute("normal")->getCount() == 36);
    REQUIRE(geometry.index == nullptr); // the pinned class never sets an index
    // The first clipped corner and its projector-space uv.
    REQUIRE(component(geometry, "position", 0) == 0.5f);
    REQUIRE(component(geometry, "position", 1) == 0.5f);
    REQUIRE(component(geometry, "position", 2) == 0.5f);
    REQUIRE(component(geometry, "uv", 0) == 1.f);
    REQUIRE(component(geometry, "uv", 1) == 1.f);
    REQUIRE(component(geometry, "uv", 2) == 1.f);
    REQUIRE(component(geometry, "uv", 3) == 0.f);
    // Every uv stays inside the projector square.
    for (std::size_t i = 0; i < geometry.getAttribute("uv")->getCount(); ++i) {
        REQUIRE(component(geometry, "uv", i) >= 0.f);
        REQUIRE(component(geometry, "uv", i) <= 1.f);
    }
}

TEST_CASE("DecalGeometry handles a geometry without normals and empty results", "[decalgeometry]") {
    DecalGeometry soup(triangleSoup(), Vector3(0, 0, 0), Euler(0, 0, 0), Vector3(1, 1, 1));
    REQUIRE(soup.getAttribute("position")->getCount() == 18);
    // A source without normals produces no normal attribute, like the pinned guard.
    REQUIRE(soup.getAttribute("normal") == nullptr);
    // A projector outside the mesh keeps the attributes but leaves them empty.
    DecalGeometry far(boxMesh(), Vector3(3, 3, 3), Euler(0, 0, 0), Vector3(1, 1, 1));
    REQUIRE(far.getAttribute("position")->getCount() == 0);
    REQUIRE(far.getAttribute("uv")->getCount() == 0);
    REQUIRE(far.getAttribute("normal") == nullptr);
}

TEST_CASE("DecalGeometry uses the mesh matrixWorld and the projector transform", "[decalgeometry]") {
    auto mesh = boxMesh();
    mesh->position.set(0.25, 0.5, -0.25);
    mesh->updateMatrixWorld(true);
    DecalGeometry translated(mesh, Vector3(0.25, 0.5, -0.25), Euler(0, 0, 0), Vector3(1, 1, 1));
    REQUIRE(translated.getAttribute("position")->getCount() == 36);
    REQUIRE(component(translated, "position", 0) == 0.75f);
    REQUIRE(component(translated, "position", 1) == 1.f);
    REQUIRE(component(translated, "position", 2) == 0.25f);
    // DecalVertex keeps an optional normal and clones both parts.
    DecalVertex plain(Vector3(1, 2, 3));
    REQUIRE_FALSE(plain.normal.has_value());
    DecalVertex clone = plain.clone();
    REQUIRE(clone.position.x == 1.);
    REQUIRE_FALSE(clone.normal.has_value());
    DecalVertex withNormal(Vector3(1, 2, 3), Vector3(0, 1, 0));
    DecalVertex copied = withNormal.clone();
    REQUIRE(copied.normal.has_value());
    REQUIRE(copied.normal->y == 1.);
    copied.position.x = 9.;
    REQUIRE(withNormal.position.x == 1.);
}
