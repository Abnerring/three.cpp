#include "renderers/common/RendererUtils.h"

#include "math/Color.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// saveRendererState( renderer, state = {} )
JSValue saveRendererState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    auto state = arg(args, 1, JSValue{});
    if (state.isUndefined() || state.isNull()) state = JSValue::object(nullptr);

    state.set("toneMapping", renderer.get("toneMapping"));
    state.set("toneMappingExposure", renderer.get("toneMappingExposure"));
    state.set("outputColorSpace", renderer.get("outputColorSpace"));
    state.set("renderTarget", invoke(renderer, "getRenderTarget")());
    state.set("activeCubeFace", invoke(renderer, "getActiveCubeFace")());
    state.set("activeMipmapLevel", invoke(renderer, "getActiveMipmapLevel")());
    state.set("renderObjectFunction", invoke(renderer, "getRenderObjectFunction")());
    state.set("pixelRatio", invoke(renderer, "getPixelRatio")());
    state.set("mrt", invoke(renderer, "getMRT")());

    // state.clearColor = renderer.getClearColor( state.clearColor || new Color() )
    auto clearColor = state.get("clearColor");
    if (!clearColor.toBoolean()) clearColor = nativeColorConstructor().construct();
    state.set("clearColor", invoke(renderer, "getClearColor")({clearColor}));

    state.set("clearAlpha", invoke(renderer, "getClearAlpha")());
    state.set("autoClear", renderer.get("autoClear"));
    state.set("scissorTest", invoke(renderer, "getScissorTest")());

    return state;
}

// resetRendererState( renderer, state ) { state = saveRendererState( renderer, state ); ... }
JSValue resetRendererState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    const auto saved = saveRendererState({}, args, {});
    auto state = saved;

    invoke(renderer, "setMRT")({JSValue(nullptr)});
    invoke(renderer, "setRenderObjectFunction")({JSValue(nullptr)});
    invoke(renderer, "setClearColor")({JSValue(0.0), JSValue(1.0)});
    auto rendererRef = renderer;
    rendererRef.set("autoClear", true);

    return state;
}

// restoreRendererState( renderer, state ) { ... }
JSValue restoreRendererState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    const auto state = arg(args, 1, nullptr);

    auto rendererRef = renderer;
    rendererRef.set("toneMapping", state.get("toneMapping"));
    rendererRef.set("toneMappingExposure", state.get("toneMappingExposure"));
    rendererRef.set("outputColorSpace", state.get("outputColorSpace"));
    invoke(renderer, "setRenderTarget")({state.get("renderTarget"), state.get("activeCubeFace"),
                                        state.get("activeMipmapLevel")});
    invoke(renderer, "setRenderObjectFunction")({state.get("renderObjectFunction")});
    invoke(renderer, "setPixelRatio")({state.get("pixelRatio")});
    invoke(renderer, "setMRT")({state.get("mrt")});
    invoke(renderer, "setClearColor")({state.get("clearColor"), state.get("clearAlpha")});
    rendererRef.set("autoClear", state.get("autoClear"));
    invoke(renderer, "setScissorTest")({state.get("scissorTest")});
    return {};
}

// saveSceneState( scene, state = {} )
JSValue saveSceneState(const JSValue&, const Args& args, const Args&) {
    const auto scene = arg(args, 0, nullptr);
    auto state = arg(args, 1, JSValue{});
    if (state.isUndefined() || state.isNull()) state = JSValue::object(nullptr);

    state.set("background", scene.get("background"));
    state.set("backgroundNode", scene.get("backgroundNode"));
    state.set("overrideMaterial", scene.get("overrideMaterial"));
    return state;
}

// resetSceneState( scene, state ) { state = saveSceneState( scene, state ); ... }
JSValue resetSceneState(const JSValue&, const Args& args, const Args&) {
    const auto scene = arg(args, 0, nullptr);
    const auto state = saveSceneState({}, args, {});

    auto sceneRef = scene;
    sceneRef.set("background", JSValue(nullptr));
    sceneRef.set("backgroundNode", JSValue(nullptr));
    sceneRef.set("overrideMaterial", JSValue(nullptr));
    return state;
}

// restoreSceneState( scene, state ) { ... }
JSValue restoreSceneState(const JSValue&, const Args& args, const Args&) {
    auto scene = arg(args, 0, nullptr);
    const auto state = arg(args, 1, nullptr);
    scene.set("background", state.get("background"));
    scene.set("backgroundNode", state.get("backgroundNode"));
    scene.set("overrideMaterial", state.get("overrideMaterial"));
    return {};
}

// saveRendererAndSceneState( renderer, scene, state = {} )
JSValue saveRendererAndSceneState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    const auto scene = arg(args, 1, nullptr);
    auto state = arg(args, 2, JSValue{});
    if (state.isUndefined() || state.isNull()) state = JSValue::object(nullptr);

    auto saved = saveRendererState({}, {renderer, state}, {});
    return saveSceneState({}, {scene, saved}, {});
}

// resetRendererAndSceneState( renderer, scene, state ) { state = resetRendererState(...); state = resetSceneState(...); return state; }
JSValue resetRendererAndSceneState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    const auto scene = arg(args, 1, nullptr);
    auto state = arg(args, 2, JSValue{});
    if (state.isUndefined() || state.isNull()) state = JSValue::object(nullptr);

    const auto rendererState = resetRendererState({}, {renderer, state}, {});
    return resetSceneState({}, {scene, rendererState}, {});
}

// restoreRendererAndSceneState( renderer, scene, state ) { restoreRendererState(...); restoreSceneState(...); }
JSValue restoreRendererAndSceneState(const JSValue&, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    const auto scene = arg(args, 1, nullptr);
    const auto state = arg(args, 2, nullptr);
    restoreRendererState({}, {renderer, state}, {});
    restoreSceneState({}, {scene, state}, {});
    return {};
}
}  // namespace

JSValue nativeRendererUtilsModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "RendererUtils module");

    auto bindings = JSValue::object(nullptr);
    // The arities are the source's: a defaulted state or scene parameter does not count.
    own(bindings, "saveRendererState",
        JSValue::function(saveRendererState, {}, "saveRendererState", 1));
    own(bindings, "resetRendererState",
        JSValue::function(resetRendererState, {}, "resetRendererState", 2));
    own(bindings, "restoreRendererState",
        JSValue::function(restoreRendererState, {}, "restoreRendererState", 2));
    own(bindings, "saveSceneState", JSValue::function(saveSceneState, {}, "saveSceneState", 1));
    own(bindings, "resetSceneState", JSValue::function(resetSceneState, {}, "resetSceneState", 2));
    own(bindings, "restoreSceneState",
        JSValue::function(restoreSceneState, {}, "restoreSceneState", 2));
    own(bindings, "saveRendererAndSceneState",
        JSValue::function(saveRendererAndSceneState, {}, "saveRendererAndSceneState", 2));
    own(bindings, "resetRendererAndSceneState",
        JSValue::function(resetRendererAndSceneState, {}, "resetRendererAndSceneState", 3));
    own(bindings, "restoreRendererAndSceneState",
        JSValue::function(restoreRendererAndSceneState, {}, "restoreRendererAndSceneState", 3));

    auto module = JSValue::object(nullptr);
    for (const auto& key : bindings.ownPropertyNames()) own(module, key.c_str(), bindings.get(key));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeRendererUtilsBinding(const std::string& name) {
    const auto keeper = nativeRendererUtilsModule();
    (void) keeper;
    return lexical().lock()->get(name);
}
