#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include "parity/MaterialClippingFixture.h"
using namespace MaterialClippingFixture;

TEST_CASE("Native Material clipping arrays share storage and replacement detaches old handles",
          "[materials][clipping]") {
    Fixture f;
    auto cppAlias = *f.source->clippingPlanes;
    REQUIRE(f.original.sameValue(f.s.get("clippingPlanes")));
    f.t.set("clippingPlanes", f.original);
    REQUIRE(f.target->clippingPlanes->sameIdentity(cppAlias));
    f.source->clippingPlanes = std::vector{f.r};
    REQUIRE_FALSE(f.s.get("clippingPlanes").sameValue(f.original));
    cppAlias.push_back(f.r);
    REQUIRE(f.original.get("length").strictEquals(3));
    REQUIRE(f.target->clippingPlanes->size() == 3);
    REQUIRE(f.source->clippingPlanes->size() == 1);
    f.original.get("0").set("constant", 18);
    REQUIRE(f.p->constant == 18);
    REQUIRE(f.r->constant == 4);
    f.s.set("clippingPlanes", nullptr);
    REQUIRE_FALSE(f.source->clippingPlanes);
    REQUIRE(f.original.get("length").strictEquals(3));
}

TEST_CASE("Native Material clipping copy clone and JSON create independent arrays",
          "[materials][clipping]") {
    Fixture f;
    auto sourceArray = *f.source->clippingPlanes;
    f.target->copy(*f.source);
    REQUIRE_FALSE(f.target->clippingPlanes->sameIdentity(sourceArray));
    REQUIRE(f.target->clippingPlanes->at(0) != f.p);
    REQUIRE(f.target->clippingPlanes->at(0)->constant == f.p->constant);
    auto clone = f.source->clone();
    REQUIRE_FALSE(clone->clippingPlanes->sameIdentity(sourceArray));
    f.source->copy(*f.source);
    REQUIRE_FALSE(f.source->clippingPlanes->sameIdentity(sourceArray));
    REQUIRE(f.original.get("0").sameValue(nativePlane(f.p)));
    auto json = f.source->toJSON();
    f.source->fromJSON(json);
    REQUIRE_FALSE(f.source->clippingPlanes->sameIdentity(sourceArray));
    REQUIRE(f.source->clippingPlanes->at(0) != f.p);
    REQUIRE(f.source->toJSON().at("clippingPlanes") == json.at("clippingPlanes"));
    f.source->clippingPlanes.emplace();
    clone = f.source->clone();
    REQUIRE(clone->clippingPlanes.has_value());
    REQUIRE(clone->clippingPlanes->empty());
    REQUIRE_FALSE(clone->toJSON().contains("clippingPlanes"));
}

TEST_CASE("Native Material enters actual WebGLClipping with live Plane and camera values",
          "[materials][clipping]") {
    Fixture f;
    auto properties = WebGLProperties().object();
    WebGLClipping clipping(properties);
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->matrixWorldInverse.makeTranslation(3, 0, 0);
    clipping.init(array(), true);
    clipping.setGlobalState(array(), nativeObject(camera));
    f.source->clipIntersection = true;
    clipping.setState(f.s, nativeObject(camera), false);
    auto scope = clipping.object(), uniform = scope.get("uniform");
    auto data = uniform.get("value");
    REQUIRE(scope.get("numPlanes").strictEquals(2));
    REQUIRE(scope.get("numIntersection").strictEquals(2));
    REQUIRE(data.get("3").strictEquals(-1));
    f.p->constant = 7;
    clipping.setState(f.s, nativeObject(camera), false);
    REQUIRE(uniform.get("value").sameValue(data));
    REQUIRE(data.get("3").strictEquals(4));
    f.source->clippingPlanes.reset();
    clipping.setState(f.s, nativeObject(camera), false);
    REQUIRE(scope.get("numPlanes").strictEquals(0));
    REQUIRE(uniform.get("value").isNull());
}

TEST_CASE("Native Material clipping rejects unsupported replacement without taking a snapshot",
          "[materials][clipping][boundary]") {
    Fixture f;
    auto ordinary = array({nativePlane(f.r)});
    REQUIRE_THROWS_AS(f.s.set("clippingPlanes", ordinary), JSTypeError);
    REQUIRE(f.s.get("clippingPlanes").sameValue(f.original));
    REQUIRE_THROWS_AS(f.s.set("clippingPlanes", false), JSTypeError);
    REQUIRE_THROWS_AS(f.original.set("0", object()), JSTypeError);
    REQUIRE(f.original.get("0").sameValue(nativePlane(f.p)));
    auto group = std::make_shared<ClippingGroup>();
    group->clippingPlanes = std::vector{f.r};
    auto shared = nativeObject(group).get("clippingPlanes");
    f.s.set("clippingPlanes", shared);
    REQUIRE(f.s.get("clippingPlanes").sameValue(shared));
    REQUIRE(f.source->clippingPlanes->sameIdentity(group->clippingPlanes));
}

TEST_CASE("Native Material owns clipping array extensions while JS handles are absent",
          "[materials][clipping][gc]") {
    auto witness = JSValue::object();
    auto owner = std::make_shared<Material>();
    owner->clippingPlanes = std::vector{makePlane(0)};
    JSWeakValue weakArray, weakExtension, weakPlane, weakNormal;
    {
        auto m = nativeMaterial(owner), a = m.get("clippingPlanes"), p = a.get("0"),
             n = p.get("normal"), extension = object({{"answer", 42}});
        a.set("extension", extension);
        p.set("extension", extension);
        n.set("extension", extension);
        weakArray = JSWeakValue(a);
        weakPlane = JSWeakValue(p);
        weakNormal = JSWeakValue(n);
        weakExtension = JSWeakValue(extension);
    }
    witness.collectGarbage();
    REQUIRE_FALSE(weakArray.expired());
    REQUIRE_FALSE(weakPlane.expired());
    REQUIRE_FALSE(weakNormal.expired());
    REQUIRE_FALSE(weakExtension.expired());
    REQUIRE(nativeMaterial(owner)
                .get("clippingPlanes")
                .get("extension")
                .get("answer")
                .strictEquals(42));
    owner->clippingPlanes = std::vector{makePlane(1)};
    witness.collectGarbage();
    REQUIRE(weakArray.expired());
    REQUIRE(weakPlane.expired());
    REQUIRE(weakNormal.expired());
    REQUIRE(weakExtension.expired());
}

TEST_CASE("External native clipping storage retains wrappers independently of replaced fields",
          "[materials][clipping][gc]") {
    auto witness = JSValue::object();
    std::optional<SharedArray<std::shared_ptr<Plane>>> alias;
    JSWeakValue weakArray, weakMaterial;
    {
        auto owner = std::make_shared<Material>();
        owner->clippingPlanes = std::vector{makePlane(0)};
        alias = *owner->clippingPlanes;
        auto m = nativeMaterial(owner), a = m.get("clippingPlanes");
        a.set("owner", m);
        weakArray = JSWeakValue(a);
        weakMaterial = JSWeakValue(m);
        owner->clippingPlanes.reset();
    }
    witness.collectGarbage();
    REQUIRE_FALSE(weakArray.expired());
    REQUIRE_FALSE(weakMaterial.expired());
    alias.reset();
    witness.collectGarbage();
    REQUIRE(weakArray.expired());
    REQUIRE(weakMaterial.expired());
}

TEST_CASE(
    "Native clipping mixed owner array plane cycles are collected after the last external handle",
    "[materials][clipping][gc]") {
    auto witness = JSValue::object();
    JSWeakValue weakMaterial, weakArray, weakPlane, weakNormal;
    std::weak_ptr<Material> nativeMaterialWeak;
    std::weak_ptr<Plane> nativePlaneWeak;
    {
        auto owner = std::make_shared<MeshBasicMaterial>();
        owner->clippingPlanes = std::vector{makePlane(0)};
        auto m = nativeMaterial(owner), a = m.get("clippingPlanes"), p = a.get("0"),
             n = p.get("normal");
        a.set("owner", m);
        p.set("owner", m);
        n.set("array", a);
        weakMaterial = JSWeakValue(m);
        weakArray = JSWeakValue(a);
        weakPlane = JSWeakValue(p);
        weakNormal = JSWeakValue(n);
        nativeMaterialWeak = owner;
        nativePlaneWeak = owner->clippingPlanes->at(0);
    }
    witness.collectGarbage();
    REQUIRE(weakMaterial.expired());
    REQUIRE(weakArray.expired());
    REQUIRE(weakPlane.expired());
    REQUIRE(weakNormal.expired());
    REQUIRE(nativeMaterialWeak.expired());
    REQUIRE(nativePlaneWeak.expired());
}
TEST_CASE(
    "Native clipping shift preserves the returned Plane and releases it after the last handle",
    "[materials][clipping][gc]") {
    auto witness = JSValue::object();
    auto owner = std::make_shared<Material>();
    owner->clippingPlanes = std::vector{makePlane(0), makePlane(1)};
    std::weak_ptr<Plane> first = owner->clippingPlanes->at(0);
    auto managed = nativeMaterial(owner), entries = managed.get("clippingPlanes");
    auto expected = entries.get("0");
    auto removed = invoke(entries, "shift");
    REQUIRE(removed.sameValue(expected));
    REQUIRE(entries.get("length").strictEquals(1));
    REQUIRE(owner->clippingPlanes->at(0)->constant == 3);
    expected = {};
    witness.collectGarbage();
    REQUIRE_FALSE(first.expired());
    REQUIRE(removed.get("constant").strictEquals(2));
    removed = {};
    witness.collectGarbage();
    REQUIRE(first.expired());
}
