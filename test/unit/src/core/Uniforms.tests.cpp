#include "catch.hpp"
#include "core/Uniform.h"
#include "core/UniformsGroup.h"
#include "core/EventDispatcher.h"
#include "constants.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arrayOf(std::initializer_list<JSValue> values) {
    auto result = JSValue::array(); for (const auto& value : values) result.get("push").call(result, {value}); return result;
}
JSPropertyDescriptor accessor(JSValue getter) {
    JSPropertyDescriptor descriptor; descriptor.getter = getter; descriptor.configurable = true; descriptor.enumerable = true; return descriptor;
}
}

TEST_CASE("Uniform preserves reference values and dynamically resolves clone", "[core][uniforms]") {
    Uniform numeric(4); auto cloned = numeric.clone(); REQUIRE_FALSE(cloned.object().sameValue(numeric.object())); REQUIRE(cloned.value.toNumber() == 4);
    auto shared = JSValue::object(); Uniform reference(shared); REQUIRE(reference.clone().value.strictEquals(shared));
    auto array = arrayOf({1, 2}); Uniform arrayUniform(array); REQUIRE(arrayUniform.clone().value.strictEquals(array));
    REQUIRE_THROWS_AS(Uniform().clone(), JSTypeError); REQUIRE_THROWS_AS(Uniform(nullptr).clone(), JSTypeError);
    auto source = reference.object(); auto replacement = JSValue::object(); replacement.set("marker", 8);
    replacement.set("clone", JSValue::function([](const JSValue& self, const Args& args, const Args&) -> JSValue {
        REQUIRE(args.empty()); return self.get("marker");
    }));
    shared.defineProperty("clone", accessor(JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue {
        auto owner = captures[0]; owner.set("value", captures[1]); return 1;
    }, {source, replacement})));
    REQUIRE(reference.clone().value.toNumber() == 8);
    auto alias = reference; alias.value = 22; REQUIRE(reference.value.toNumber() == 22);
    auto moved = std::move(alias); moved.value = 23; REQUIRE(reference.value.toNumber() == 23);
}

TEST_CASE("UniformsGroup descriptors defaults chaining and shared arrays match upstream", "[core][uniforms]") {
    UniformsGroup a, b;
    REQUIRE(a.isUniformsGroup.toBoolean()); REQUIRE(a.name.toString().empty()); REQUIRE(a.usage.toNumber() == StaticDrawUsage);
    REQUIRE(b.id.toNumber() == a.id.toNumber() + 1);
    auto descriptor = a.object().getOwnPropertyDescriptor("id"); REQUIRE(descriptor); REQUIRE_FALSE(*descriptor->writable); REQUIRE_FALSE(*descriptor->enumerable); REQUIRE_FALSE(*descriptor->configurable);
    REQUIRE_THROWS_AS(a.id = 19, JSTypeError);
    REQUIRE(a.object().getPrototype().sameValue(UniformsGroup::prototype()));
    REQUIRE(UniformsGroup::prototype().getPrototype().sameValue(EventDispatcher::runtimePrototype()));
    REQUIRE(UniformsGroup::constructor().getPrototype().sameValue(EventDispatcher::runtimePrototype().get("constructor")));
    Uniform u(7); REQUIRE(a.add(u).sameValue(a.object())); a.add(u); a.add(9); a.remove(u);
    REQUIRE(a.uniforms.get("length").toNumber() == 2); REQUIRE(a.uniforms.get("0").sameValue(u.object()));
    REQUIRE(a.setName(12).sameValue(a.object())); REQUIRE(a.name.toNumber() == 12);
    REQUIRE(a.setUsage(nullptr).sameValue(a.object())); REQUIRE(a.usage.isNull());
    b.uniforms = a.uniforms.value(); b.copy(a); REQUIRE(a.uniforms.get("length").toNumber() == 0); REQUIRE(b.uniforms.strictEquals(a.uniforms.value()));
}

TEST_CASE("UniformsGroup copy preserves sparse nested arrays and partial failure", "[core][uniforms]") {
    UniformsGroup source, destination; Uniform first(2), third(3);
    auto nested = JSValue::array(3); nested.set("0", first); nested.set("2", third);
    source.add(first); source.add(nested); source.add(nullptr);
    REQUIRE_THROWS_AS(destination.copy(source), JSTypeError);
    REQUIRE(destination.uniforms.get("length").toNumber() == 2);
    REQUIRE_FALSE(destination.uniforms.get("0").sameValue(first.object()));
    auto copied = destination.uniforms.get("1"); REQUIRE(copied.length() == 3); REQUIRE_FALSE(copied.hasOwn("1")); REQUIRE(copied.get("2").get("value").toNumber() == 3);
    REQUIRE(source.uniforms.get("length").toNumber() == 3);
    source.copy(source); REQUIRE(source.uniforms.get("length").toNumber() == 0);
}

TEST_CASE("UniformsGroup copy captures push before a clone replaces the target array", "[core][uniforms]") {
    UniformsGroup source, destination; auto retained = destination.uniforms.value(); auto replacement = JSValue::array();
    auto item = JSValue::object(); item.set("clone", JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue {
        auto destination = captures[0]; destination.set("uniforms", captures[1]); return 8;
    }, {destination.object(), replacement}));
    source.add(item); source.add(Uniform(9)); destination.copy(source);
    REQUIRE(retained.length() == 1); REQUIRE(retained.get("0").toNumber() == 8);
    REQUIRE(replacement.length() == 1); REQUIRE(replacement.get("0").get("value").toNumber() == 9);
    REQUIRE(destination.uniforms.strictEquals(replacement));
}

TEST_CASE("UniformsGroup clone observes constructor and dynamic copy without silent fallback", "[core][uniforms]") {
    UniformsGroup source; source.add(Uniform(4)); auto clone = source.clone(); REQUIRE_FALSE(clone.sameValue(source.object()));
    REQUIRE(clone.get("id").toNumber() == source.id.toNumber() + 1); REQUIRE(clone.get("uniforms").get("0").get("value").toNumber() == 4);
    auto observed = JSValue::array(); source.object().defineProperty("constructor", accessor(JSValue::function([](const JSValue&, const Args&, const Args& captures) -> JSValue {
        captures[0].get("push").call(captures[0], {"constructor"}); return nullptr;
    }, {observed})));
    REQUIRE_THROWS_AS(source.clone(), JSTypeError); REQUIRE(observed.length() == 1);
    auto prototype = UniformsGroup::prototype(); auto original = prototype.get("copy");
    prototype.set("copy", JSValue::function([](const JSValue& self, const Args& args, const Args&) -> JSValue { return arrayOf({self.get("id"), args[0].get("name")}); }));
    UniformsGroup other; other.setName("dynamic"); auto result = other.clone(); prototype.set("copy", original);
    REQUIRE(result.type() == JSValue::Type::Array); REQUIRE(result.get("1").toString() == "dynamic");
}

TEST_CASE("Array map snapshots length preserves holes and observes inherited indices", "[core][uniforms]") {
    auto array = JSValue::array(4), proto = JSValue::object(array.getPrototype()); proto.set("1", 20); array.setPrototype(proto); array.set("0", 10); array.set("3", 40);
    auto receiver = JSValue::object(), trace = JSValue::array();
    auto result = array.get("map").call(array, {JSValue::function([](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        REQUIRE(self.sameValue(captures[0])); REQUIRE(args.size() == 3);
        auto array = args[2]; auto index = args[1].toNumber(); captures[1].get("push").call(captures[1], {index});
        if (index == 0) { array.erase("3"); array.set("4", 50); }
        return args[0].toNumber() * 2;
    }, {receiver, trace}), receiver});
    REQUIRE(result.length() == 4); REQUIRE(result.get("0").toNumber() == 20); REQUIRE(result.get("1").toNumber() == 40);
    REQUIRE_FALSE(result.hasOwn("2")); REQUIRE_FALSE(result.hasOwn("3")); REQUIRE(trace.length() == 2);
}

TEST_CASE("UniformsGroup event and callback cycles are collected while external handles remain valid", "[core][uniforms][gc]") {
    auto root = JSValue::object(); Uniform::prototype(); UniformsGroup::prototype(); root.collectGarbage(); const auto baseline = root.heapObjectCount();
    std::weak_ptr<int> weak;
    {
        UniformsGroup group; auto payload = std::make_shared<int>(42); weak = payload;
        auto host = JSValue::hostObject(payload, payload.get(), "UniformLifetimeSentinel");
        auto value = JSValue::object(); value.set("owner", group); value.set("host", host);
        Uniform uniform(value); group.add(uniform);
        auto listener = JSValue::function([](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            REQUIRE(self.sameValue(captures[0])); REQUIRE(args[0].get("target").sameValue(self));
            self.collectGarbage(); captures[1].get("push").call(captures[1], {args[0]}); return JSValue();
        }, {group.object(), JSValue::array()});
        group.addEventListener("dispose", listener); group.addEventListener("dispose", listener); REQUIRE(group.hasEventListener("dispose", listener).toBoolean());
        REQUIRE(group.dispose().isUndefined()); REQUIRE(group.uniforms.get("length").toNumber() == 1);
        root.collectGarbage(); REQUIRE_FALSE(weak.expired());
    }
    REQUIRE(root.collectGarbage() > 0); REQUIRE(weak.expired()); REQUIRE(root.heapObjectCount() == baseline);
}
