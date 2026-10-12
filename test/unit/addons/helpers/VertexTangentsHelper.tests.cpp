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

TEST_CASE("Vertex normals and tangents use inverse-transpose and forward transforms", "[addons][helpers]") {
    auto source = vertexSource();
    source->scale.set(2, 1, 3);
    source->position.set(4, 5, 6);
    VertexNormalsHelper normals(source, 2);
    VertexTangentsHelper tangents(source, 2);
    const auto n = normals.geometry->getAttribute("position");
    const auto t = tangents.geometry->getAttribute("position");
    REQUIRE(n->getX(0) == 4);
    REQUIRE(n->getY(0) == 5);
    REQUIRE(n->getZ(0) == 6);
    REQUIRE(n->getX(1) == Approx(4 + 2 / std::sqrt(5)).epsilon(1e-6));
    REQUIRE(n->getY(1) == Approx(5 + 4 / std::sqrt(5)).epsilon(1e-6));
    REQUIRE(t->getX(1) == Approx(4 + 4 / std::sqrt(5)).epsilon(1e-6));
    REQUIRE(t->getY(1) == Approx(5 + 2 / std::sqrt(5)).epsilon(1e-6));
    source->geometry->getAttribute("tangent")->setW(0, 1);
    tangents.update();
    REQUIRE(t->getX(1) == Approx(4 + 4 / std::sqrt(5)).epsilon(1e-6));
}


TEST_CASE("Vertex helpers consume interleaved attributes through VertexAttribute", "[addons][helpers]") {
    Float32Array values{1,2,3, 0,1,0, 1,0,0,-1};
    InterleavedBuffer data(values, 10);
    auto source = std::make_shared<Mesh>();
    source->geometry->setAttribute("position", std::make_shared<InterleavedBufferAttribute>(data.object(),3,0));
    source->geometry->setAttribute("normal", std::make_shared<InterleavedBufferAttribute>(data.object(),3,3));
    source->geometry->setAttribute("tangent", std::make_shared<InterleavedBufferAttribute>(data.object(),4,6));
    VertexNormalsHelper normals(source);
    VertexTangentsHelper tangents(source);
    const auto n = normals.geometry->getAttribute("position");
    const auto t = tangents.geometry->getAttribute("position");
    REQUIRE(n->getX(0) == 1);
    REQUIRE(n->getY(1) == 3);
    REQUIRE(t->getX(1) == 2);
    REQUIRE(t->getY(1) == 2);
}


TEST_CASE("Vertex helper sizes retain JS coercion order and undefined defaults", "[addons][helpers]") {
    auto source = vertexSource();
    auto counter = JSValue::object();
    counter.set("count",0);
    auto size = JSValue::object();
    size.set("valueOf",JSValue::function(scalar,{counter}));
    VertexTangentsHelper helper(source,size);
    REQUIRE(helper.size.sameValue(size));
    REQUIRE(counter.get("count").number() == 6);
    helper.update();
    REQUIRE(counter.get("count").number() == 12);
    VertexNormalsHelper defaults(source,JSValue{},HelperColor(JSValue{}));
    REQUIRE(defaults.size.number() == 1);
    REQUIRE(Helper_detail::lineColor(defaults.material).getHex() == 0xff0000);
    defaults.size = "2";
    REQUIRE_NOTHROW(defaults.update());
}

