#include "catch.hpp"
#include "parity/LayersUniformsFixture.h"
#include <thread>
using namespace LayersUniformsFixture;
TEST_CASE("UniformsUtils export literal shares mutable function identities", "[uniforms-utils]") {
    auto legacy = nativeUniformsUtils(), clone = nativeCloneUniforms(),
         merge = nativeMergeUniforms();
    REQUIRE(legacy.sameValue(nativeUniformsUtils()));
    REQUIRE(legacy.get("clone").sameValue(clone));
    REQUIRE(legacy.get("merge").sameValue(merge));
    REQUIRE(clone.isConstructible());
    REQUIRE(clone.get("name").strictEquals("cloneUniforms"));
    REQUIRE(clone.get("length").strictEquals(1));
    auto replacement = JSValue::function(
        +[](const JSValue&, const LayerArgs&, const LayerArgs&) -> JSValue { return 7; });
    legacy.set("clone", replacement);
    REQUIRE(legacy.get("clone").sameValue(replacement));
    REQUIRE(nativeCloneUniforms().sameValue(clone));
    REQUIRE(clone.call({}, {}).isObject());
    legacy.set("clone", clone);
    auto other = JSValue::constructor(
        +[](const JSValue&, const LayerArgs&, const LayerArgs&) -> JSValue { return {}; }, {},
        "Other", 0, JSValue::ConstructorKind::Callable);
    other.set("prototype", JSValue::object());
    auto cloned = clone.construct({}, other);
    REQUIRE_FALSE(cloned.getPrototype().sameValue(other.get("prototype")));
}
TEST_CASE("UniformsUtils warning executes the shared callback before null assignment",
          "[uniforms-utils]") {
    auto source = JSValue::object(), uniform = JSValue::object(), texture = JSValue::object(),
         events = list();
    source.set("u", uniform);
    texture.set("isTexture", true);
    texture.set("isRenderTargetTexture", true);
    uniform.set("value", texture);
    ThreeUtils::setConsoleFunction(JSValue::function(
        +[](const JSValue& self, const LayerArgs& args, const LayerArgs& c) -> JSValue {
            log(c[0], list({self.isUndefined(), args.size(), args[0], args[1]}));
            return {};
        },
        {events}));
    auto result = nativeCloneUniforms().call({}, {source});
    ThreeUtils::setConsoleFunction({});
    REQUIRE(result.get("u").get("value").isNull());
    REQUIRE(events.get("0").get("0").strictEquals(true));
    REQUIRE(events.get("0").get("1").strictEquals(2));
    REQUIRE(events.get("0").get("2").strictEquals("warn"));
    ThreeUtils::setConsoleFunction(
        JSValue::function(+[](const JSValue&, const LayerArgs&, const LayerArgs&) -> JSValue {
            throw JSRangeError("warning");
            return {};
        }));
    REQUIRE_THROWS_AS(cloneUniforms(source), JSRangeError);
    ThreeUtils::setConsoleFunction({});
}
TEST_CASE("UniformsUtils module and detached function do not own their heap",
          "[uniforms-utils][gc]") {
    std::exception_ptr failure;
    std::thread fresh([&] {
        try {
            JSWeakValue legacy, clone, payload;
            std::weak_ptr<int> native;
            {
                auto realm = JSValue::object();
                auto exportObject = nativeUniformsUtils();
                auto value = std::make_shared<int>(3);
                native = value;
                auto host = JSValue::hostObject(value, value.get(), "uniforms.exports.Witness");
                payload = JSWeakValue(host);
                legacy = JSWeakValue(exportObject);
                clone = JSWeakValue(nativeCloneUniforms());
                exportObject.set("witness", host);
                host.set("module", exportObject);
                value.reset();
                host = {};
                exportObject = {};
                realm.collectGarbage();
                REQUIRE_FALSE(legacy.expired());
                REQUIRE_FALSE(native.expired());
            }
            REQUIRE(legacy.expired());
            REQUIRE(clone.expired());
            REQUIRE(payload.expired());
            REQUIRE(native.expired());
            REQUIRE_FALSE(nativeUniformsUtils().hasOwn("witness"));
        } catch (...) {
            failure = std::current_exception();
        }
    });
    fresh.join();
    if (failure)
        std::rethrow_exception(failure);
}
