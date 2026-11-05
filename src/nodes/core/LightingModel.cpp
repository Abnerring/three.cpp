#include "nodes/core/LightingModel.h"

#include "nodes/core/LightingModelSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue start(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto setup = invoke(builder.get("lightsNode"), "setupLights");
    setup({builder, invoke(builder.get("lightsNode"), "getLightNodes")({builder})});
    invoke(self, "indirect")({builder});
    return {};
}
JSValue emptyHook(const JSValue&, const Args&, const Args&) { return {}; }
}

JSValue nativeLightingModelModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    auto module = JSValue::object(nullptr);
    auto ctor = JSValue::constructor(
        +[](const JSValue& target, const Args&, const Args&) -> JSValue { return target; },
        {}, "LightingModel", 0);
    ctor.setFunctionSourceText(LightingModelSource::classSource);
    const auto prototype = ctor.get("prototype");
    method(prototype, "start", 1, start, {}, LightingModelSource::startSource);
    method(prototype, "finish", 0, emptyHook, {}, LightingModelSource::finishSource);
    method(prototype, "direct", 0, emptyHook, {}, LightingModelSource::directSource);
    method(prototype, "directRectArea", 0, emptyHook, {}, LightingModelSource::directRectAreaSource);
    method(prototype, "indirect", 0, emptyHook, {}, LightingModelSource::indirectSource);
    method(prototype, "ambientOcclusion", 0, emptyHook, {}, LightingModelSource::ambientOcclusionSource);
    own(module, "default", ctor);
    return publish(module, lexical(), weak);
}
JSValue nativeLightingModelBinding(const std::string& name) {
    const auto keeper = nativeLightingModelModule();
    (void)keeper;
    return lexical().lock()->get(name);
}
JSValue nativeLightingModelConstructor() { return nativeLightingModelBinding("default"); }

std::vector<LightingModel::Hook> LightingModel::hooks() {
    return {Hook::Start, Hook::Finish, Hook::Direct, Hook::DirectRectArea, Hook::Indirect,
            Hook::AmbientOcclusion};
}

bool LightingModel::hasBody(Hook hook) {
    // Only the first hook has anything of its own to do.
    return hook == Hook::Start;
}

LightingModel::Caller LightingModel::callerOf(Hook hook) {
    if (hook == Hook::Start || hook == Hook::Finish || hook == Hook::Direct ||
        hook == Hook::DirectRectArea) {
        return Caller::Framework;
    }
    if (hook == Hook::Indirect) {
        // The indirect term is reached through the first hook rather than directly.
        return Caller::TheFirstHook;
    }
    // The ambient occlusion one is reached by nobody automatically.
    return Caller::Nobody;
}

std::vector<std::string> LightingModel::startSteps() {
    // The direct lights first, then the indirect term.
    return {"setupLights", "indirect"};
}

std::string LightingModel::ambientOcclusionNote() {
    return "Unlike other methods, this method must be called manually by the lighting model in its "
           "indirect term.";
}
