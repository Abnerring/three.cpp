#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/Raycaster.h"
#include "geometries/BoxGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Plane.h"
#include "objects/Mesh.h"
#include "textures/Texture.h"

TEST_CASE("Attribute upload callbacks retain function state and survive self replacement", "[attribute]") {
    Float32BufferAttribute attribute({1, 2, 3}, 1);
    std::vector<int> calls;
    attribute.onUpload([state = 0, &attribute, &calls]() mutable {
        calls.push_back(++state);
        if (state == 2) attribute.onUpload([&calls] { calls.push_back(10); });
        calls.push_back(state);
    });
    auto callback = attribute.onUploadCallback.value();
    callback.call(); attribute.invokeUploadCallback(); attribute.invokeUploadCallback(); callback.call();
    REQUIRE(calls == std::vector<int>{1, 1, 2, 2, 10, 3, 3});
    REQUIRE_FALSE(callback.sameValue(attribute.onUploadCallback.value()));
}

TEST_CASE("Write-only update flags do not copy another object's owner", "[attribute][texture]") {
    Float32BufferAttribute a({1}, 1), b({2}, 1);
    a.needsUpdate = b.needsUpdate;
    REQUIRE(JSValue(a.needsUpdate).isUndefined());
    a.needsUpdate = true;
    REQUIRE(a.version.toNumber() == 1);
    REQUIRE(b.version.toNumber() == 0);
    Texture first, second;
    first.needsUpdate = second.needsUpdate;
    first.needsPMREMUpdate = first.needsUpdate;
    first.needsUpdate = true;
    first.needsPMREMUpdate = true;
    REQUIRE(first.version == 1);
    REQUIRE(second.version == 0);
    REQUIRE(first.pmremVersion == 1);
    first.source->needsUpdate = second.source->needsUpdate;
    first.source->needsUpdate = a.needsUpdate;
    first.source->needsUpdate = true;
    REQUIRE(first.source->version == 2);
    REQUIRE(second.source->version == 0);
    a.updateRanges = nullptr; first.updateRanges.reset();
    REQUIRE_THROWS_AS(a.addUpdateRange(0, 1), JSTypeError);
    REQUIRE_THROWS_AS(a.clearUpdateRanges(), JSTypeError);
    REQUIRE_THROWS_AS(first.addUpdateRange(0, 1), JSTypeError);
    REQUIRE_THROWS_AS(first.clearUpdateRanges(), JSTypeError);
}

TEST_CASE("Material alphaTest assignment keeps destination version ownership", "[material]") {
    MeshBasicMaterial a, b;
    b.alphaTest = 0.5;
    a.alphaTest = b.alphaTest;
    REQUIRE(double(a.alphaTest) == 0.5);
    REQUIRE(a.version == 1);
    REQUIRE(b.version == 1);
    a.alphaTest = 0;
    REQUIRE(a.version == 2);
    REQUIRE(b.version == 1);
}

TEST_CASE("Float64 normalized components retain upstream invalid type behavior", "[attribute]") {
    Float64BufferAttribute attribute({0.5}, 1, true);
    REQUIRE_THROWS_AS(attribute.getX(0), std::invalid_argument);
    REQUIRE_THROWS_AS(attribute.setX(0, 1), std::invalid_argument);
    attribute.normalized = false;
    REQUIRE(attribute.getX(0) == 0.5);
}

TEST_CASE("Material empty clipping planes clone without missing Plane dependency", "[material]") {
    MeshBasicMaterial material;
    material.clippingPlanes.emplace();
    auto copy = material.clone();
    REQUIRE(copy->clippingPlanes.has_value());
    REQUIRE(copy->clippingPlanes->empty());
    REQUIRE_FALSE(copy->toJSON().contains("clippingPlanes"));
}

TEST_CASE("toNonIndexed uses raw index data despite normalized flag", "[geometry]") {
    BufferGeometry geometry;
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(
                                          std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8, 9}, 3));
    geometry.setIndex(
        std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{2, 0, 1}, 1, true));
    auto expanded = geometry.toNonIndexed()->getAttribute("position");
    REQUIRE(expanded->getX(0) == 7);
    REQUIRE(expanded->getX(1) == 1);
    REQUIRE(expanded->getX(2) == 4);
}

TEST_CASE("BufferAttribute preserves cross-item access and numeric missing components",
          "[attribute]") {
    Float32BufferAttribute attribute({1, 2, 3, 4, 5, 6}, 3);
    REQUIRE(attribute.getW(0) == 4);
    REQUIRE(std::isnan(attribute.getW(1)));
    attribute.setW(0, 7).setW(1, 9).setX(std::numeric_limits<std::size_t>::max(), 10);
    REQUIRE(attribute.getX(1) == 7);
    REQUIRE(attribute.count == 2);
    REQUIRE(std::isnan(attribute.getX(std::numeric_limits<std::size_t>::max())));
}

TEST_CASE("Material clipping plane clone and self-copy preserve independent plane references",
          "[material]") {
    MeshBasicMaterial material;
    auto plane = std::make_shared<Plane>(Vector3(0, 1, 0), -2);
    material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{plane};
    auto clone = material.clone();
    REQUIRE(clone->clippingPlanes->at(0).get() != plane.get());
    plane->constant = -3;
    REQUIRE(clone->clippingPlanes->at(0)->constant == -2);
    REQUIRE(material.toJSON().contains("clippingPlanes"));
    material.copy(material);
    REQUIRE(material.clippingPlanes->at(0).get() != plane.get());
    REQUIRE(material.clippingPlanes->at(0)->constant == -3);
}

TEST_CASE("Mesh queries ignore invalid materials on excluded groups", "[raycast]") {
    auto geometry = std::make_shared<BoxGeometry>();
    geometry->setDrawRange(0, 6);
    Mesh mesh(geometry,
              std::vector<std::shared_ptr<Material>>{std::make_shared<MeshBasicMaterial>()});
    mesh.updateMatrixWorld(true);
    auto hits = Raycaster({2, .1, .2}, {-1, 0, 0}).intersectObject(mesh);
    REQUIRE(hits.size() == 1);
    REQUIRE(hits.front().face->materialIndex == 0);
    geometry->setDrawRange(0, 0);
    mesh.materials[0].reset();
    REQUIRE(Raycaster({2, .1, .2}, {-1, 0, 0}).intersectObject(mesh).empty());
}

TEST_CASE("Raycaster retains managed cameras and intersection object references", "[raycast]") {
    Raycaster query({.1, .2, 2}, {0, 0, -1});
    std::weak_ptr<Object3D> weakMesh;
    std::vector<Intersection> results;
    {
        auto mesh = std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(2, 2));
        weakMesh = mesh;
        mesh->updateMatrixWorld(true);
        results = query.intersectObject(*mesh);
    }
    REQUIRE(results.size() == 1);
    REQUIRE_FALSE(weakMesh.expired());
    results.clear();
    REQUIRE(weakMesh.expired());
    std::weak_ptr<Camera> weakCamera;
    {
        auto camera = std::make_shared<PerspectiveCamera>();
        weakCamera = camera;
        camera->updateMatrixWorld(true);
        query.setFromCamera({0, 0}, *camera);
    }
    REQUIRE_FALSE(weakCamera.expired());
    REQUIRE(query.camera->isCamera);
}

TEST_CASE("Raycaster captures input length during custom callbacks", "[raycast]") {
    struct Appender : Object3D {
        SharedArray<std::shared_ptr<Object3D>>* objects = nullptr;
        bool raycast(const Raycaster&, std::vector<Intersection>&) override {
            objects->push_back(std::make_shared<Object3D>());
            return true;
        }
    };
    auto appender = std::make_shared<Appender>();
    SharedArray<std::shared_ptr<Object3D>> objects{appender, std::make_shared<Object3D>()};
    appender->objects = &objects;
    REQUIRE(Raycaster().intersectObjects(objects).empty());
    REQUIRE(objects.size() == 3);
}

TEST_CASE("Mesh query keeps captured material alive while a vertex override replaces it",
          "[raycast]") {
    struct ReplacingMesh : Mesh {
        using Mesh::Mesh;
        mutable bool observed = true;
        std::weak_ptr<Material> original;
        Vector3& getVertexPosition(std::size_t index, Vector3& target) const override {
            const_cast<ReplacingMesh*>(this)->material.reset();
            observed = observed && !original.expired();
            return Mesh::getVertexPosition(index, target);
        }
    };
    ReplacingMesh mesh(std::make_shared<PlaneGeometry>(2, 2));
    mesh.original = mesh.material;
    mesh.updateMatrixWorld(true);
    auto hits = Raycaster({.1, .2, 2}, {0, 0, -1}).intersectObject(mesh);
    REQUIRE(mesh.observed);
    REQUIRE(hits.size() == 1);
    REQUIRE(mesh.original.expired());
}
