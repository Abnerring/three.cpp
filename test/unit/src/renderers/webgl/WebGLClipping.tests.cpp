#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include "parity/WebGLClippingFixture.h"
#include <thread>
using namespace ClippingFixture;

TEST_CASE("WebGLClipping global local shadow cache state follows frame transitions",
          "[webgl-clipping]") {
    Fixture f;
    REQUIRE(f.call("init", {f.globals, true}).strictEquals(true));
    f.call("setGlobalState", {f.globals, f.camera});
    auto global = f.uniform.get("value");
    REQUIRE(global.get("length").strictEquals(4));
    REQUIRE(global.get("0").strictEquals(1));
    REQUIRE(global.get("3").strictEquals(2));
    f.material.set("clipIntersection", true);
    f.call("setState", {f.material, f.camera, false});
    auto local = f.cache.get("clippingState");
    REQUIRE(local.get("length").strictEquals(12));
    REQUIRE(f.manager.get("numPlanes").strictEquals(3));
    REQUIRE(f.manager.get("numIntersection").strictEquals(2));
    f.local.set("0", plane(0));
    f.call("setState", {f.material, f.camera, true});
    REQUIRE(f.cache.get("clippingState").sameValue(local));
    REQUIRE(local.get("4").number() == Approx(1. / std::sqrt(14.)));
    f.call("beginShadows");
    REQUIRE(f.manager.get("numPlanes").strictEquals(0));
    REQUIRE(f.uniform.get("value").sameValue(local));
    f.material.set("clipShadows", true);
    f.call("setState", {f.material, f.camera, false});
    REQUIRE(f.manager.get("numPlanes").strictEquals(2));
    REQUIRE(f.uniform.get("value").sameValue(local));
    f.call("endShadows");
    f.material.set("clippingPlanes", nullptr);
    f.call("setState", {f.material, f.camera});
    REQUIRE(f.uniform.get("value").sameValue(global));
    REQUIRE(f.manager.get("numPlanes").strictEquals(1));
    REQUIRE(f.manager.get("numIntersection").strictEquals(0));
}

TEST_CASE("WebGLClipping native Plane and camera adapters project actual "
          "numeric storage",
          "[webgl-clipping]") {
    Fixture f;
    auto p = std::make_shared<Plane>(Vector3(1, 0, 0), 2);
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->matrixWorldInverse.makeTranslation(3, 0, 0);
    auto planes = array({nativePlane(p)});
    f.call("init", {planes, false});
    f.call("setGlobalState", {planes, nativeObject(camera)});
    auto result = f.uniform.get("value");
    REQUIRE(result.get("0").strictEquals(1));
    REQUIRE(result.get("3").strictEquals(-1));
    p->constant = 5;
    f.call("setGlobalState", {planes, nativeObject(camera)});
    REQUIRE(f.uniform.get("value").sameValue(result));
    REQUIRE(result.get("3").strictEquals(2));
    REQUIRE(p->normal.x == 1);
    REQUIRE(p->constant == 5);
}

TEST_CASE("WebGLClipping strict cache flag and short buffers preserve upstream "
          "behavior",
          "[webgl-clipping]") {
    Fixture f;
    f.call("init", {array(), true});
    f.call("setGlobalState", {array(), f.camera});
    auto buffer = NativeArray_detail::constructor(6).construct({1});
    buffer.set("0", 99);
    f.cache.set("clippingState", buffer);
    f.call("setState", {f.material, nullptr, true});
    REQUIRE(f.uniform.get("value").sameValue(buffer));
    REQUIRE(buffer.get("0").strictEquals(99));
    REQUIRE(f.manager.get("numPlanes").strictEquals(2));
    REQUIRE_THROWS_AS(f.call("setState", {f.material, nullptr, "true"}), JSTypeError);
    f.call("setState", {f.material, f.camera, "true"});
    REQUIRE_FALSE(f.uniform.get("value").sameValue(buffer));
    REQUIRE(f.uniform.get("value").get("length").strictEquals(8));
}

TEST_CASE("WebGLClipping detached receiver and replaced public uniform "
          "preserve closure identity",
          "[webgl-clipping]") {
    Fixture f;
    f.call("init", {f.globals, true});
    f.call("setGlobalState", {f.globals, f.camera});
    auto replacement = object({{"value", 17}});
    f.manager.set("uniform", replacement);
    f.material.set("clipIntersection", true);
    auto receiver = object({{"numPlanes", 10}});
    f.manager.get("setState").call(receiver, {f.material, f.camera, false});
    REQUIRE(f.manager.get("uniform").sameValue(replacement));
    REQUIRE(replacement.get("value").strictEquals(17));
    REQUIRE(f.manager.get("numPlanes").strictEquals(2));
    REQUIRE(receiver.get("numPlanes").strictEquals(11));
    REQUIRE(receiver.get("numIntersection").strictEquals(10));
    REQUIRE(f.uniform.get("value").sameValue(f.cache.get("clippingState")));
}

TEST_CASE("WebGLClipping keeps partial writes when a plane getter throws", "[webgl-clipping]") {
    Fixture f;
    f.call("init", {array(), true});
    f.call("setGlobalState", {array(), f.camera});
    auto buffer = NativeArray_detail::constructor(6).construct({8});
    f.cache.set("clippingState", buffer);
    auto bad = plane(1);
    JSPropertyDescriptor d;
    d.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue {
        throw JSRangeError("constant");
    });
    bad.defineProperty("constant", d);
    f.local.set("0", plane(0));
    f.local.set("1", bad);
    REQUIRE_THROWS_AS(f.call("setState", {f.material, f.camera, false}), JSRangeError);
    REQUIRE(buffer.get("0").strictEquals(1));
    REQUIRE(buffer.get("3").strictEquals(2));
    REQUIRE(buffer.get("4").strictEquals(0));
    REQUIRE(f.manager.get("numPlanes").strictEquals(0));
    REQUIRE_FALSE(f.uniform.get("needsUpdate").toBoolean());
    REQUIRE(f.uniform.get("value").sameValue(buffer));
}

TEST_CASE("WebGLClipping captures form managed cycles and detached methods "
          "retain their owner",
          "[webgl-clipping][gc]") {
    auto witness = JSValue::object();
    JSWeakValue weakManager, weakProperties, weakUniform;
    JSValue method;
    {
        auto properties = WebGLProperties().object();
        auto manager = WebGLClipping(properties).object();
        weakManager = JSWeakValue(manager);
        weakProperties = JSWeakValue(properties);
        weakUniform = JSWeakValue(manager.get("uniform"));
        method = manager.get("beginShadows");
    }
    witness.collectGarbage();
    REQUIRE_FALSE(weakManager.expired());
    REQUIRE_FALSE(weakProperties.expired());
    REQUIRE_FALSE(weakUniform.expired());
    method.call();
    REQUIRE(weakManager.lock()->get("numPlanes").strictEquals(0));
    method = {};
    witness.collectGarbage();
    REQUIRE(weakManager.expired());
    REQUIRE(weakProperties.expired());
    REQUIRE(weakUniform.expired());
}

TEST_CASE("WebGLClipping rejects foreign managed inputs before copying them",
          "[webgl-clipping][threads]") {
    auto foreign = JSValue::object();
    WebGLClipping owner;
    unsigned rejected = 0, accepted = 0;
    std::thread worker([&] {
        const auto reject = [&](auto action) {
            try {
                action();
                ++accepted;
            } catch (const std::logic_error&) {
                ++rejected;
            }
        };
        auto localValue = JSValue::object();
        WebGLClipping local;
        reject([&] { WebGLClipping bad(foreign); });
        reject([&] { static_cast<void>(WebGLClipping::fromObject(foreign)); });
        reject([&] { static_cast<void>(owner.object()); });
        reject([&] { static_cast<void>(static_cast<JSValue>(owner)); });
        reject([&] { WebGLClipping copy(owner); });
        reject([&] { WebGLClipping moved(std::move(owner)); });
        reject([&] { local = owner; });
        reject([&] { local = std::move(owner); });
        reject([&] { owner = local; });
        reject([&] { owner = std::move(local); });
        reject([&] { owner.beginShadows(); });
        reject([&] { owner.endShadows(); });
        reject([&] { owner.init(); });
        reject([&] { owner.setGlobalState(); });
        reject([&] { owner.setState(); });
        reject([&] { local.init(foreign); });
        reject([&] { local.init(localValue, foreign); });
        reject([&] { local.setGlobalState(foreign); });
        reject([&] { local.setGlobalState(localValue, foreign); });
        reject([&] { local.setState(foreign); });
        reject([&] { local.setState(localValue, foreign); });
        reject([&] { local.setState(localValue, localValue, foreign); });
    });
    worker.join();
    REQUIRE(accepted == 0);
    REQUIRE(rejected == 22);
    REQUIRE(foreign.keys().empty());
    REQUIRE(owner.object().get("numPlanes").strictEquals(0));
}
