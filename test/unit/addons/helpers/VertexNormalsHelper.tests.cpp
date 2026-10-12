#include "catch.hpp"
#include "helpers/VertexNormalsHelper.h"
#include "helpers/VertexTangentsHelper.h"
#include "core/InterleavedBufferAttribute.h"
#include "objects/Points.h"

namespace {
std::shared_ptr<Mesh> vertexSource() {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<double>{0,0,0, 1,2,3}, 3));
    geometry->setAttribute("normal", std::make_shared<Float32BufferAttribute>(
        std::vector<double>{1,1,0, 0,1,0}, 3));
    geometry->setAttribute("tangent", std::make_shared<Float32BufferAttribute>(
        std::vector<double>{1,1,0,-1, 0,1,0,1}, 4));
    return std::make_shared<Mesh>(geometry);
}
JSValue scalar(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
    auto counter = captures[0];
    const auto count = counter.get("count").toNumber() + 1;
    counter.set("count", count);
    return count;
}
struct UpdatingMesh : Mesh {
    using Mesh::Mesh;
    std::function<void()> after;
    void updateMatrixWorld(bool force = false) override {
        Mesh::updateMatrixWorld(force);
        if (after) after();
    }
};
}

TEST_CASE("Vertex helpers preserve source identity and distinct upstream defaults", "[addons][helpers]") {
    auto source = vertexSource();
    VertexNormalsHelper normals(source);
    VertexTangentsHelper tangents(source);
    REQUIRE(normals.object == source);
    REQUIRE(tangents.object == source);
    REQUIRE(normals.isVertexNormalsHelper);
    REQUIRE(normals.isLineSegments);
    REQUIRE(tangents.isLineSegments);
    REQUIRE(normals.type == "VertexNormalsHelper");
    REQUIRE(tangents.type == "VertexTangentsHelper");
    REQUIRE_FALSE(normals.matrixAutoUpdate);
    REQUIRE_FALSE(tangents.matrixAutoUpdate);
    REQUIRE(normals.size.number() == 1);
    REQUIRE(Helper_detail::lineColor(normals.material).getHex() == 0xff0000);
    REQUIRE(Helper_detail::lineColor(tangents.material).getHex() == 0x00ffff);
    REQUIRE_FALSE(normals.material->toneMapped);
    REQUIRE_FALSE(tangents.material->toneMapped);
    REQUIRE(normals.geometry->getAttribute("position")->getCount() == 4);
    REQUIRE(normals.geometry->getAttribute("position")->getProperty("version").number() == 1);
}


TEST_CASE("Vertex helper updates keep fixed allocation and ignore indices and draw ranges", "[addons][helpers]") {
    auto source = vertexSource();
    source->geometry->setIndex(std::vector<std::uint32_t>{1});
    source->geometry->setDrawRange(1, 0);
    VertexNormalsHelper helper(source);
    auto position = helper.geometry->getAttribute("position");
    REQUIRE(position->getCount() == 4);
    REQUIRE(position->getX(2) == 1);
    source->geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<double>{0,0,0, 1,2,3, 4,5,6}, 3));
    helper.update();
    REQUIRE(helper.geometry->getAttribute("position") == position);
    REQUIRE(position->getCount() == 4);
    REQUIRE(position->getProperty("version").number() == 2);
    helper.geometry->computeBoundingSphere();
    const auto previousSphere = *helper.geometry->boundingSphere;
    helper.size = 8;
    helper.update();
    REQUIRE(helper.geometry->boundingSphere->center.equals(previousSphere.center));
    REQUIRE(helper.geometry->boundingSphere->radius == previousSphere.radius);
}


TEST_CASE("Vertex helper missing geometry behavior and exceptions match upstream", "[addons][helpers]") {
    REQUIRE_THROWS_AS(VertexNormalsHelper(), JSTypeError);
    REQUIRE_THROWS_AS(VertexTangentsHelper(), JSTypeError);
    auto source = vertexSource();
    VertexNormalsHelper normals(source);
    VertexTangentsHelper tangents(source);
    source->geometry.reset();
    REQUIRE_NOTHROW(normals.update());
    REQUIRE(normals.geometry->getAttribute("position")->getProperty("version").number() == 2);
    REQUIRE_THROWS_AS(tangents.update(), JSTypeError);
    REQUIRE(tangents.geometry->getAttribute("position")->getProperty("version").number() == 1);
    normals.object.reset();
    REQUIRE_THROWS_AS(normals.update(), JSTypeError);
}


TEST_CASE("Empty positions defer missing normal and tangent reads until loop execution", "[addons][helpers]") {
    auto source = vertexSource();
    VertexNormalsHelper normals(source);
    VertexTangentsHelper tangents(source);
    source->geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(0, 3));
    source->geometry->deleteAttribute("normal");
    source->geometry->deleteAttribute("tangent");
    REQUIRE_NOTHROW(normals.update());
    REQUIRE_NOTHROW(tangents.update());
    source->geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(3, 3));
    REQUIRE_THROWS_AS(normals.update(), JSTypeError);
    REQUIRE_THROWS_AS(tangents.update(), JSTypeError);
}


TEST_CASE("Vertex helper update re-reads source after updateMatrixWorld reentry", "[addons][helpers]") {
    auto old = std::make_shared<UpdatingMesh>(vertexSource()->geometry);
    auto next = vertexSource();
    next->matrixWorld.makeTranslation(7,8,9);
    VertexNormalsHelper helper(old);
    old->after = [&] { helper.object = next; };
    helper.update();
    REQUIRE(helper.object == next);
    REQUIRE(helper.geometry->getAttribute("position")->getX(0) == 7);
    REQUIRE(helper.geometry->getAttribute("position")->getY(0) == 8);
}


TEST_CASE("Vertex helper inherited copy omits addon fields and clone fails without source", "[addons][helpers]") {
    auto a = vertexSource(), b = vertexSource();
    VertexNormalsHelper first(a,2), second(b,4);
    REQUIRE(&first.copy(second,false) == &first);
    REQUIRE(first.geometry == second.geometry);
    REQUIRE(first.material == second.material);
    REQUIRE(first.object == a);
    REQUIRE(first.size.number() == 2);
    REQUIRE_THROWS_AS(first.clone(),JSTypeError);
    REQUIRE_THROWS_AS(first.clone(false),JSTypeError);
    VertexTangentsHelper tangents(a);
    REQUIRE_THROWS_AS(tangents.clone(),JSTypeError);
    REQUIRE_THROWS_AS(tangents.clone(false),JSTypeError);
}


TEST_CASE("Vertex helper dispose emits only resources and observes callback replacement", "[addons][helpers]") {
    VertexNormalsHelper helper(vertexSource());
    std::vector<int> events;
    helper.addEventListener("dispose",[&](Event&) { events.push_back(0); });
    auto replacement = std::make_shared<LineBasicMaterial>();
    helper.geometry->addEventListener("dispose",[&](Event&) { events.push_back(1); helper.material = replacement; });
    helper.material->addEventListener("dispose",[&](Event&) { events.push_back(2); });
    replacement->addEventListener("dispose",[&](Event&) { events.push_back(3); });
    helper.dispose();
    helper.dispose();
    REQUIRE(events == std::vector<int>{1,3,1,3});
}
