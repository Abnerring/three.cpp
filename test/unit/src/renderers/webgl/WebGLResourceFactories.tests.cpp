#include "catch.hpp"
#include "renderers/webgl/WebGLMorphtargets.h"
#include "renderers/webgl/WebGLEnvironments.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "textures/Texture.h"
#include "textures/DataTexture.h"
#include <array>
#include <stdexcept>

namespace {
using Args = std::vector<JSValue>;
JSValue call(JSValue receiver, const char* method, const Args& args = {}) { return receiver.get(method).call(receiver, args); }
JSValue list(const Args& values = {}) { auto result = JSValue::array(); for (const auto& value : values) result.set(JSValue(result.length()), value); return result; }
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> fields = {}) {
    auto result = JSValue::object(); for (const auto& [name, value] : fields) result.set(name, value); return result;
}
struct MorphProtocol {
    JSValue gl = JSValue::object(), textures = JSValue::object();
    JSValue values = JSValue::object(), calls = JSValue::array(), uniforms = JSValue::object(), program = JSValue::object();
    JSValue geometry = nativeBufferGeometryConstructor().construct();
    JSValue influence = list({.25}), instance = object({{"morphTargetInfluences", influence}});
    JSValue position = nativeFloat32BufferAttributeConstructor().construct({list({0, 0, 0, 0, 0, 0}), 3});
    JSValue target = nativeFloat32BufferAttributeConstructor().construct({list({1, 2, 3, 4, 5, 6}), 3});
    WebGLMorphtargets manager;
    MorphProtocol() : manager(gl, object({{"maxTextureSize", 1}}), textures) {
        call(geometry, "setAttribute", {"position", position}); geometry.get("morphAttributes").set("position", list({target}));
        // Only this CPU protocol's final uniform-command boundary is recorded.
        // Textures, buffer attributes, vectors and cache entries are canonical.
        uniforms.set("setValue", JSValue::function(+[](const JSValue&, const Args& args, const Args& c) {
            auto values = c[0], events = c[1]; values.set(args.at(1), args.at(2));
            events.set(JSValue(events.length()), list(args)); return JSValue();
        }, {values, calls}));
        program.set("getUniforms", JSValue::function(+[](const JSValue&, const Args&, const Args& c) { return c[0]; }, {uniforms}));
    }
    void update() { call(manager.object(), "update", {instance, geometry, program}); }
};
}

TEST_CASE("Original environment factory preserves null and passthrough identities and exposes unavailable dependencies", "[webgl][environments][runtime]") {
    const auto ctor = nativeWebGLEnvironmentsConstructor();
    REQUIRE(ctor.sameValue(nativeWebGLEnvironmentsConstructor()));
    REQUIRE(ctor.get("length").strictEquals(1)); REQUIRE(ctor.get("name").strictEquals("WebGLEnvironments"));
    WebGLEnvironments environments;
    REQUIRE(call(environments.object(), "get").isNull()); REQUIRE(call(environments.object(), "get", {nullptr}).isNull());
    for (const auto& value : Args{false, 0, "", object()}) REQUIRE(call(environments.object(), "get", {value}).sameValue(value));
    auto texture = nativeTextureConstructor().construct();
    REQUIRE(call(environments.object(), "get", {texture}).sameValue(texture));
    REQUIRE(call(environments.object(), "get", {texture, true}).sameValue(texture));
    texture.set("mapping", EquirectangularReflectionMapping);
    REQUIRE(call(environments.object(), "get", {texture}).isNull());
    texture.set("image", object({{"height", 2}}));
    REQUIRE_THROWS_AS(call(environments.object(), "get", {texture}), std::logic_error);
    REQUIRE_THROWS_AS(call(environments.object(), "get", {texture, true}), std::logic_error);
    texture.set("isPMREMTexture", true);
    REQUIRE(call(environments.object(), "get", {texture, true}).sameValue(texture));
    REQUIRE(call(environments.object(), "dispose").isUndefined());
    REQUIRE(call(environments.object(), "get", {nullptr, true}).isNull());
}
TEST_CASE("Original morph factory packs the actual data texture and retains source cache behavior", "[webgl][morphtargets][runtime]") {
    const auto ctor = nativeWebGLMorphtargetsConstructor();
    REQUIRE(ctor.sameValue(nativeWebGLMorphtargetsConstructor())); REQUIRE(ctor.get("length").strictEquals(3));
    MorphProtocol context; context.update();
    const auto value = context.values.get("morphTargetsTexture"), size = context.values.get("morphTargetsTextureSize");
    REQUIRE(value.get("isDataArrayTexture").strictEquals(true)); REQUIRE(value.get("version").strictEquals(1));
    REQUIRE(value.get("type").strictEquals(FloatType)); REQUIRE(size.get("x").strictEquals(1)); REQUIRE(size.get("y").strictEquals(2));
    REQUIRE(context.values.get("morphTargetBaseInfluence").strictEquals(.75));
    REQUIRE(context.values.get("morphTargetInfluences").sameValue(context.influence));
    const auto image = value.get("image"), data = image.get("data");
    REQUIRE(image.get("width").strictEquals(1)); REQUIRE(image.get("height").strictEquals(2)); REQUIRE(image.get("depth").strictEquals(1));
    const std::array<double, 8> expected{1, 2, 3, 0, 4, 5, 6, 0};
    REQUIRE(data.get("length").strictEquals(double(expected.size())));
    for (unsigned i = 0; i < expected.size(); ++i) REQUIRE(data.get(JSValue(double(i))).strictEquals(expected[i]));
    REQUIRE(context.calls.length() == 4);
    REQUIRE(context.calls.get("2").get("0").sameValue(context.gl));
    REQUIRE(context.calls.get("2").get("3").sameValue(context.textures));
    auto targetData = context.target.get("array"); targetData.set("0", 9); context.update();
    REQUIRE(context.values.get("morphTargetsTexture").sameValue(value));
    REQUIRE(data.get("0").strictEquals(1)); // Source repacks on target count changes, not attribute mutation.
    context.geometry.set("morphTargetsRelative", true); context.update();
    REQUIRE(context.values.get("morphTargetBaseInfluence").strictEquals(1));
}
TEST_CASE("Original morph count replacement and disposal retain closure identities and rebuild the cache", "[webgl][morphtargets][runtime]") {
    MorphProtocol context; context.update();
    const auto first = context.values.get("morphTargetsTexture");
    auto counts = object({{"first", 0}, {"second", 0}});
    auto listener = JSValue::function(+[](const JSValue&, const Args&, const Args& c) {
        auto state = c[0]; state.set(c[1], state.get(c[1]).number() + 1); return JSValue();
    }, {counts, "first"});
    call(first, "addEventListener", {"dispose", listener});
    context.geometry.get("morphAttributes").set("position", list({context.target, context.target})); context.update();
    const auto second = context.values.get("morphTargetsTexture");
    REQUIRE_FALSE(first.sameValue(second)); REQUIRE(counts.get("first").strictEquals(1)); REQUIRE(second.get("image").get("depth").strictEquals(2));
    call(second, "addEventListener", {"dispose", JSValue::function(+[](const JSValue&, const Args&, const Args& c) {
        auto state = c[0]; state.set("second", state.get("second").number() + 1); return JSValue();
    }, {counts})});
    call(context.geometry, "dispose");
    REQUIRE(counts.get("first").strictEquals(2)); // The older source geometry closure is still registered until disposal.
    REQUIRE(counts.get("second").strictEquals(1));
    context.update(); REQUIRE_FALSE(context.values.get("morphTargetsTexture").sameValue(second));
    call(context.geometry, "dispose"); REQUIRE(counts.get("first").strictEquals(2)); REQUIRE(counts.get("second").strictEquals(1));
}
