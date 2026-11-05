#include "renderers/common/RendererUtils.h"

#include "nodes/lighting/LightingNodeRuntime.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue tagged(const char* tag) {
    auto value = JSValue::object(nullptr);
    value.set("tag", JSValue(tag));
    return value;
}

// Installs a stub renderer that records its calls into the given log array.
JSValue makeRenderer(const JSValue& log) {
    auto renderer = JSValue::object(nullptr);
    renderer.set("toneMapping", 2);
    renderer.set("toneMappingExposure", 1.5);
    renderer.set("outputColorSpace", "srgb");
    renderer.set("autoClear", false);

    const auto getter = [&](const char* name, const JSValue& value) {
        renderer.set(name, JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                auto entries = captures[0];
                entries.set(entries.length(), captures[1]);
                return captures[2];
            },
            {log, JSValue(name), value}, name, 0));
    };
    const auto setter = [&](const char* name, int arity) {
        renderer.set(name, JSValue::function(
            +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
                auto entries = captures[0];
                entries.set(entries.length(), captures[1]);
                (void) args;
                return {};
            },
            {log, JSValue(name)}, name, arity));
    };
    getter("getRenderTarget", tagged("rt"));
    getter("getActiveCubeFace", JSValue(1));
    getter("getActiveMipmapLevel", JSValue(2));
    getter("getRenderObjectFunction", tagged("rof"));
    getter("getPixelRatio", JSValue(2));
    getter("getMRT", tagged("mrt"));
    getter("getClearAlpha", JSValue(0.25));
    getter("getScissorTest", JSValue(true));
    renderer.set("getClearColor", JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            auto entries = captures[0];
            entries.set(entries.length(), JSValue("getClearColor"));
            auto color = args.size() > 0 ? args[0] : JSValue{};
            color.set("r", 0.5); color.set("g", 0.25); color.set("b", 0.125);
            return color;
        },
        {log}, "getClearColor", 1));
    setter("setMRT", 1);
    setter("setRenderObjectFunction", 1);
    setter("setClearColor", 2);
    setter("setRenderTarget", 3);
    setter("setPixelRatio", 1);
    setter("setScissorTest", 1);
    return renderer;
}

JSValue makeScene() {
    auto scene = JSValue::object(nullptr);
    scene.set("background", tagged("bg"));
    scene.set("backgroundNode", tagged("bgn"));
    scene.set("overrideMaterial", tagged("om"));
    return scene;
}
}  // namespace

TEST_CASE("RendererUtils exports the nine state helpers with their source arities") {
    const auto module = nativeRendererUtilsModule();
    struct Entry {
        const char* name;
        double length;
    };
    const Entry entries[] = {
        {"saveRendererState", 1},   {"resetRendererState", 2},   {"restoreRendererState", 2},
        {"saveSceneState", 1},      {"resetSceneState", 2},      {"restoreSceneState", 2},
        {"saveRendererAndSceneState", 2}, {"resetRendererAndSceneState", 3},
        {"restoreRendererAndSceneState", 3}};
    for (const auto& entry : entries) {
        REQUIRE(module.get(entry.name).isCallable());
        // A defaulted parameter does not count towards the arity.
        REQUIRE(module.get(entry.name).get("length").toNumber() == Approx(entry.length));
    }
    REQUIRE(module.get("notAHelper").isUndefined());
}

TEST_CASE("RendererUtils saves the renderer and scene state into one object") {
    auto log = JSValue::array();
    const auto renderer = makeRenderer(log);
    const auto scene = makeScene();
    const auto state =
        nativeRendererUtilsBinding("saveRendererAndSceneState").call({}, {renderer, scene});

    REQUIRE(state.get("toneMapping").toNumber() == Approx(2));
    REQUIRE(state.get("toneMappingExposure").toNumber() == Approx(1.5));
    REQUIRE(state.get("outputColorSpace").strictEquals("srgb"));
    REQUIRE(state.get("renderTarget").get("tag").strictEquals("rt"));
    REQUIRE(state.get("activeCubeFace").toNumber() == Approx(1));
    REQUIRE(state.get("activeMipmapLevel").toNumber() == Approx(2));
    REQUIRE(state.get("renderObjectFunction").get("tag").strictEquals("rof"));
    REQUIRE(state.get("pixelRatio").toNumber() == Approx(2));
    REQUIRE(state.get("mrt").get("tag").strictEquals("mrt"));
    // The clear colour is written into the colour handed to getClearColor.
    REQUIRE(state.get("clearColor").get("r").toNumber() == Approx(0.5));
    REQUIRE(state.get("clearColor").get("b").toNumber() == Approx(0.125));
    REQUIRE(state.get("clearAlpha").toNumber() == Approx(0.25));
    REQUIRE(state.get("autoClear").strictEquals(false));
    REQUIRE(state.get("scissorTest").strictEquals(true));
    REQUIRE(state.get("background").get("tag").strictEquals("bg"));
    REQUIRE(state.get("backgroundNode").get("tag").strictEquals("bgn"));
    REQUIRE(state.get("overrideMaterial").get("tag").strictEquals("om"));
}

TEST_CASE("RendererUtils reset clears the renderer and the scene and answers the saved state") {
    auto log = JSValue::array();
    const auto renderer = makeRenderer(log);
    const auto scene = makeScene();
    auto state = JSValue::object(nullptr);
    const auto returned =
        nativeRendererUtilsBinding("resetRendererAndSceneState").call({}, {renderer, scene, state});

    // The state object is filled in by the save that the reset performs first.
    REQUIRE(returned.sameValue(state));
    REQUIRE(state.get("toneMapping").toNumber() == Approx(2));
    REQUIRE(state.get("background").get("tag").strictEquals("bg"));
    // Then the defaults are written.
    REQUIRE(renderer.get("autoClear").strictEquals(true));
    REQUIRE(scene.get("background").isNull());
    REQUIRE(scene.get("backgroundNode").isNull());
    REQUIRE(scene.get("overrideMaterial").isNull());
}

TEST_CASE("RendererUtils restore puts the saved values back on the renderer and the scene") {
    auto log = JSValue::array();
    const auto renderer = makeRenderer(log);
    const auto scene = makeScene();
    const auto state =
        nativeRendererUtilsBinding("saveRendererAndSceneState").call({}, {renderer, scene});

    // Change everything the state covers, then restore.
    auto rendererRef = renderer;
    rendererRef.set("toneMapping", 9);
    rendererRef.set("autoClear", true);
    auto sceneRef = scene;
    sceneRef.set("background", JSValue(nullptr));
    sceneRef.set("overrideMaterial", JSValue(nullptr));

    nativeRendererUtilsBinding("restoreRendererAndSceneState").call({}, {renderer, scene, state});
    REQUIRE(renderer.get("toneMapping").toNumber() == Approx(2));
    REQUIRE(renderer.get("toneMappingExposure").toNumber() == Approx(1.5));
    REQUIRE(renderer.get("outputColorSpace").strictEquals("srgb"));
    REQUIRE(renderer.get("autoClear").strictEquals(false));
    REQUIRE(scene.get("background").get("tag").strictEquals("bg"));
    REQUIRE(scene.get("overrideMaterial").get("tag").strictEquals("om"));
}
