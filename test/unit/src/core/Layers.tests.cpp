#include "catch.hpp"
#include "parity/LayersUniformsFixture.h"
#include <thread>
using namespace LayersUniformsFixture;
TEST_CASE("Layers retains arbitrary shared masks and exact signed results", "[layers]") {
    Layers layer;
    auto view = layer.object();
    REQUIRE(layer.mask == 1);
    layer.set(31);
    REQUIRE(layer.mask == 2147483648.);
    layer.enable(0);
    REQUIRE(layer.mask == -2147483647.);
    layer.enableAll();
    REQUIRE(layer.mask == -1);
    layer.mask = "9";
    REQUIRE(view.get("mask").strictEquals("9"));
    REQUIRE(layer.isEnabled(0));
    auto arbitrary = JSValue::object();
    arbitrary.set("tag", "mask");
    layer.mask = arbitrary;
    REQUIRE(view.get("mask").sameValue(arbitrary));
    Layers alias = layer;
    alias.mask = 7;
    REQUIRE(layer.mask == 7);
    Layers moved = std::move(alias);
    moved.set(2);
    REQUIRE(layer.mask == 4);
    auto old = view;
    layer = Layers();
    REQUIRE_FALSE(layer.object().sameValue(old));
    REQUIRE(old.get("mask").strictEquals(4));
}
TEST_CASE("Layers callback evaluation order and inherited writes follow the class", "[layers]") {
    auto self = JSValue::object(), events = list(), mask = JSValue::object(),
         arg = JSValue::object();
    mask.set("valueOf", JSValue::function(
                            +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                                log(c[0], "mask");
                                return 1;
                            },
                            {events}));
    arg.set("valueOf", JSValue::function(
                           +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                               log(c[0], "layer");
                               auto self = c[1];
                               self.set("mask", 99);
                               return 31;
                           },
                           {events, self}));
    self.set("mask", mask);
    Layers::prototype().get("enable").call(self, {arg});
    REQUIRE(self.get("mask").strictEquals(-2147483647.));
    REQUIRE(events.get("0").strictEquals("layer"));
    REQUIRE(events.get("1").strictEquals("mask"));
    const auto method = Layers::prototype().get("test");
    auto other = JSValue::object();
    getter(other, "mask",
           JSValue::function(
               +[](const JSValue&, const LayerArgs&, const LayerArgs& c) -> JSValue {
                   log(c[0], "right");
                   return 1;
               },
               {events}));
    self.set("mask", mask);
    method.call(self, {other});
    REQUIRE(events.get("2").strictEquals("right"));
    REQUIRE(events.get("3").strictEquals("mask"));
}
TEST_CASE("Native Object3D layers replacement preserves aliases copy and JSON", "[layers]") {
    auto source = std::make_shared<Object3D>(), destination = std::make_shared<Object3D>();
    auto wrapper = nativeObject(source), old = wrapper.get("layers");
    auto replacement = Layers().object();
    replacement.set("mask", "12");
    wrapper.set("layers", replacement);
    REQUIRE(source->layers.object().sameValue(replacement));
    old.set("mask", 7);
    REQUIRE(source->layers.mask.value().strictEquals("12"));
    source->layers.mask = 19;
    REQUIRE(replacement.get("mask").strictEquals(19));
    auto target = destination->layers.object();
    destination->copy(*source, false);
    REQUIRE(destination->layers.object().sameValue(target));
    REQUIRE_FALSE(target.sameValue(replacement));
    REQUIRE(destination->layers.mask == 19);
    REQUIRE(destination->toJSON().at("object").at("layers") == JsonValue(19));
    auto obj = JSValue::object();
    obj.set("tag", "arbitrary");
    source->layers.mask = obj;
    destination->copy(*source, false);
    REQUIRE(destination->layers.mask.value().sameValue(obj));
    source->layers.mask = {};
    REQUIRE_FALSE(source->toJSON().at("object").contains("layers"));
    Raycaster rays;
    auto raysOld = rays.layers.object();
    rays.layers = Layers::fromObject(replacement);
    REQUIRE(rays.layers.object().sameValue(replacement));
    REQUIRE(raysOld.get("mask").strictEquals(1));
}
TEST_CASE("Native layers fields participate in native cycle collection", "[layers][gc]") {
    auto realm = JSValue::object();
    auto owner = std::make_shared<Object3D>();
    auto wrapper = nativeObject(owner), layer = wrapper.get("layers");
    JSWeakValue wrapperWeak(wrapper), layerWeak(layer);
    std::weak_ptr<Object3D> ownerWeak = owner;
    auto opaque = std::make_shared<int>(8);
    std::weak_ptr<int> native = opaque;
    auto mask = JSValue::hostObject(opaque, opaque.get(), "layers.GCWitness");
    mask.set("back", wrapper);
    layer.set("mask", mask);
    wrapper = {};
    layer = {};
    mask = {};
    opaque.reset();
    realm.collectGarbage();
    REQUIRE_FALSE(wrapperWeak.expired());
    REQUIRE_FALSE(native.expired());
    owner.reset();
    realm.collectGarbage();
    REQUIRE(ownerWeak.expired());
    REQUIRE(wrapperWeak.expired());
    REQUIRE(layerWeak.expired());
    REQUIRE(native.expired());
    owner = std::make_shared<Object3D>();
    wrapper = nativeObject(owner);
    auto old = wrapper.get("layers");
    auto witness = std::make_shared<int>(5);
    std::weak_ptr<int> oldNative = witness;
    auto oldMask = JSValue::hostObject(witness, witness.get(), "layers.OldWitness");
    oldMask.set("back", old);
    old.set("mask", oldMask);
    owner->layers = Layers();
    oldMask = {};
    witness.reset();
    realm.collectGarbage();
    REQUIRE_FALSE(oldNative.expired());
    old = {};
    realm.collectGarbage();
    REQUIRE(oldNative.expired());
}
TEST_CASE("Layers shared module objects release their heap and reject foreign references",
          "[layers][threads]") {
    std::exception_ptr failure;
    std::thread fresh([&] {
        try {
            JSWeakValue ctor, layer;
            {
                auto realm = JSValue::object();
                Layers value;
                ctor = JSWeakValue(Layers::constructor());
                layer = JSWeakValue(value.object());
            }
            REQUIRE(ctor.expired());
            REQUIRE(layer.expired());
        } catch (...) {
            failure = std::current_exception();
        }
    });
    fresh.join();
    if (failure)
        std::rethrow_exception(failure);
    Layers local;
    auto foreign = JSValue::object();
    std::vector<bool> rejected;
    std::thread other([&] {
        auto own = JSValue::object();
        for (const auto& fn : std::vector<std::function<void()>>{
                 [&] { local.object(); }, [&] { Layers copy(local); },
                 [&] { Layers copy(std::move(local)); }, [&] { local.mask.value(); },
                 [&] { local.mask = own; }, [&] { local.set(own); }, [&] { local.enable(1); },
                 [&] { Layers::fromObject(foreign); },
                 [&] {
                     Layers native;
                     native.mask = foreign;
                 },
                 [&] { local = Layers(); }}) {
            try {
                fn();
                rejected.push_back(false);
            } catch (const std::logic_error&) {
                rejected.push_back(true);
            }
        }
    });
    other.join();
    REQUIRE(rejected.size() == 10);
    for (const auto r : rejected)
        REQUIRE(r);
    REQUIRE(local.mask == 1);
}

TEST_CASE("Native Layers method getter captures camera layers after lookup", "[layers]") {
    const auto normal = dispatch(0), replacement = dispatch(1), throwing = dispatch(2);
    REQUIRE(normal.at("result") == JsonValue(true));
    REQUIRE(replacement.at("result") == JsonValue(true));
    REQUIRE(replacement.at("leftMask") == JsonValue(1));
    REQUIRE(throwing.at("error") == JsonValue("RangeError"));
    REQUIRE(throwing.at("rightMask") == JsonValue(2));
}

TEST_CASE("Object3D copy retains the assignment base across RHS layer replacement", "[layers]") {
    REQUIRE(copyReentry(0).at("oldMask") == JsonValue(19));
    const auto swapped = copyReentry(1), nullTarget = copyReentry(2), throwing = copyReentry(3);
    REQUIRE(swapped.at("oldMask") == JsonValue(19));
    REQUIRE(swapped.at("current") == JsonValue(5));
    REQUIRE(nullTarget.at("error") == JsonValue(""));
    REQUIRE(nullTarget.at("oldMask") == JsonValue(19));
    REQUIRE(throwing.at("oldMask") == JsonValue(1));
    REQUIRE(throwing.at("error") == JsonValue("RangeError"));
}
