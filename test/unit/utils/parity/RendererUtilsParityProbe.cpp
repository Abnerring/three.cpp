#include "renderers/common/RendererUtils.h"

#include "core/JSValue.h"
#include "nodes/lighting/LightingNodeRuntime.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

// The stub renderer and scene record every call, so the differential compares the exact call sequence
// the helpers make rather than only their return values.
std::string describe(const JSValue& value) {
    if (value.isNull()) return "null";
    if (value.type() == JSValue::Type::Number) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%g", value.toNumber());
        return buffer;
    }
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean() ? "true" : "false";
    if (value.isObject()) {
        const auto tag = value.get("tag");
        if (!tag.isUndefined()) return tag.string();
    }
    return "?";
}

// JSValue is a handle, so taking the log by value still appends to the same array object.
void record(JSValue log, const std::string& entry) {
    log.set(log.length(), JSValue(entry));
}

JSValue tagged(const char* tag) {
    auto value = JSValue::object(nullptr);
    value.set("tag", JSValue(tag));
    return value;
}
}  // namespace

int main() {
    try {
        const auto module = nativeRendererUtilsModule();
        const auto binding = [&](const char* name) { return nativeRendererUtilsBinding(name); };

        auto log = JSValue::array();
        auto renderer = JSValue::object(nullptr);
        renderer.set("toneMapping", 2);
        renderer.set("toneMappingExposure", 1.5);
        renderer.set("outputColorSpace", "srgb");
        renderer.set("autoClear", false);
        auto getter = [&](const char* name, const JSValue& value) {
            renderer.set(name, JSValue::function(
                +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                    record(captures[1], captures[0].string());
                    return captures[2];
                },
                {JSValue(name), log, value}, name, 0));
        };
        auto setter = [&](const char* name, int arity) {
            renderer.set(name, JSValue::function(
                +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
                    std::string entry = captures[0].string() + "(";
                    for (size_t i = 0; i < args.size(); ++i) {
                        if (i > 0) entry += ",";
                        entry += describe(args[i]);
                    }
                    record(captures[1], entry + ")");
                    return {};
                },
                {JSValue(name), log}, name, arity));
        };
        getter("getRenderTarget", tagged("rt"));
        getter("getActiveCubeFace", JSValue(1));
        getter("getActiveMipmapLevel", JSValue(2));
        getter("getRenderObjectFunction", tagged("rof"));
        getter("getPixelRatio", JSValue(2));
        getter("getMRT", tagged("mrt"));
        getter("getClearAlpha", JSValue(0.25));
        getter("getScissorTest", JSValue(true));
        // getClearColor( color ) writes into the colour it is handed and answers it.
        renderer.set("getClearColor", JSValue::function(
            +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
                record(captures[0], "getClearColor");
                auto color = args.size() > 0 ? args[0] : JSValue{};
                color.set("tag", JSValue("cc"));
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

        auto scene = JSValue::object(nullptr);
        scene.set("background", tagged("bg"));
        scene.set("backgroundNode", tagged("bgn"));
        scene.set("overrideMaterial", tagged("om"));

        const auto serializeState = [](const JSValue& state) {
            auto out = JSValue::object(nullptr);
            for (const char* key : {"toneMapping", "toneMappingExposure", "outputColorSpace",
                                    "renderTarget", "activeCubeFace", "activeMipmapLevel",
                                    "renderObjectFunction", "pixelRatio", "mrt", "clearAlpha",
                                    "autoClear", "scissorTest", "background", "backgroundNode",
                                    "overrideMaterial"}) {
                out.set(key, JSValue(describe(state.get(key))));
            }
            const auto clearColor = state.get("clearColor");
            auto color = JSValue::object(nullptr);
            color.set("r", clearColor.get("r"));
            color.set("g", clearColor.get("g"));
            color.set("b", clearColor.get("b"));
            out.set("clearColor", color);
            return out;
        };
        const auto takeLog = [](const JSValue& entries) {
            auto out = JSValue::array();
            for (unsigned i = 0; i < entries.length(); ++i) out.set(i, entries.get(i));
            return out;
        };
        const auto clearLog = [&]() { log.set("length", JSValue(0)); };

        // 1. Save both, then check the state and the calls it made.
        const auto savedState = binding("saveRendererAndSceneState").call({}, {renderer, scene});
        auto savedFields = serializeState(savedState);
        auto saveCalls = takeLog(log);
        clearLog();

        // 2. Reset both into the saved state object, which re-saves first and then clears.
        const auto resetState =
            binding("resetRendererAndSceneState").call({}, {renderer, scene, savedState});
        auto resetFields = serializeState(resetState);
        auto resetCalls = takeLog(log);
        auto afterReset = JSValue::object(nullptr);
        afterReset.set("autoClear", renderer.get("autoClear"));
        afterReset.set("sceneBackground", describe(scene.get("background")));
        afterReset.set("sceneBackgroundNode", describe(scene.get("backgroundNode")));
        afterReset.set("sceneOverrideMaterial", describe(scene.get("overrideMaterial")));
        clearLog();

        // 3. Restore both from the state the first save produced.
        binding("restoreRendererAndSceneState").call({}, {renderer, scene, savedFields.isObject() ? savedState : savedState});
        auto restoreCalls = takeLog(log);
        auto afterRestore = JSValue::object(nullptr);
        afterRestore.set("toneMapping", renderer.get("toneMapping"));
        afterRestore.set("toneMappingExposure", renderer.get("toneMappingExposure"));
        afterRestore.set("outputColorSpace", renderer.get("outputColorSpace"));
        afterRestore.set("autoClear", renderer.get("autoClear"));
        afterRestore.set("sceneBackground", describe(scene.get("background")));
        afterRestore.set("sceneOverrideMaterial", describe(scene.get("overrideMaterial")));

        auto lengths = JSValue::object(nullptr);
        for (const char* name : {"saveRendererState", "resetRendererState", "restoreRendererState",
                                 "saveSceneState", "resetSceneState", "restoreSceneState",
                                 "saveRendererAndSceneState", "resetRendererAndSceneState",
                                 "restoreRendererAndSceneState"}) {
            lengths.set(name, binding(name).get("length"));
        }

        auto output = JSValue::object(nullptr);
        output.set("lengths", lengths);
        output.set("savedFields", savedFields);
        output.set("saveCalls", saveCalls);
        output.set("resetFields", resetFields);
        output.set("resetCalls", resetCalls);
        output.set("afterReset", afterReset);
        output.set("restoreCalls", restoreCalls);
        output.set("afterRestore", afterRestore);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
