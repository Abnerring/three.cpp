#include "nodes/accessors/SceneProperties.h"

#include "constants.h"
#include "math/Matrix4.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue scalarUpdate(const JSValue&, const Args& args, const Args& captures) {
    const auto scene = arg(args, 0).get("scene");
    return scene.get(captures[0].toPropertyKey());
}

JSValue rotationUpdate(const JSValue&, const Args& args, const Args& captures) {
    const auto scene = arg(args, 0).get("scene");
    const auto background = scene.get("background");
    const bool textureCase = !background.isNull() && !background.isUndefined() &&
        background.get("isTexture").strictEquals(true) &&
        !background.get("mapping").strictEquals(UVMapping);
    const auto backgroundNode = scene.get("backgroundNode");
    const bool nodeCase = backgroundNode.toBoolean() && backgroundNode.get("isNode").strictEquals(true);
    auto matrix = captures[0];
    if (textureCase || nodeCase) {
        invoke(matrix, "makeRotationFromEuler")({scene.get("backgroundRotation")});
        invoke(matrix, "transpose")();
    } else {
        invoke(matrix, "identity")();
    }
    return matrix;
}

JSValue callback(JSValue::NativeFunction function, const Args& captures, const char* source) {
    auto result = JSValue::function(function, captures, "", 1);
    result.setFunctionSourceText(source);
    return result;
}
}

JSValue nativeScenePropertiesModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SceneProperties module");

    const auto uniform = nativeUniformFactory();
    const auto renderGroup = nativeUniformGroupNodeBinding("renderGroup");
    const auto matrix = nativeMatrix4Constructor().construct();
    auto bindings = JSValue::object(nullptr);

    auto blurriness = invoke(uniform.call({}, {0}), "setGroup")({renderGroup});
    blurriness = invoke(blurriness, "onRenderUpdate")({callback(scalarUpdate, {"backgroundBlurriness"}, "( { scene } ) => scene.backgroundBlurriness")});
    own(bindings, "backgroundBlurriness", blurriness);

    auto intensity = invoke(uniform.call({}, {1}), "setGroup")({renderGroup});
    intensity = invoke(intensity, "onRenderUpdate")({callback(scalarUpdate, {"backgroundIntensity"}, "( { scene } ) => scene.backgroundIntensity")});
    own(bindings, "backgroundIntensity", intensity);

    auto rotation = invoke(uniform.call({}, {matrix}), "setGroup")({renderGroup});
    rotation = invoke(rotation, "onRenderUpdate")({callback(rotationUpdate, {matrix}, "( { scene } ) => {\n\n\tconst background = scene.background;\n\n\tif ( ( background !== null && background.isTexture && background.mapping !== UVMapping ) || ( scene.backgroundNode && scene.backgroundNode.isNode ) ) {\n\n\t\t_m1.makeRotationFromEuler( scene.backgroundRotation ).transpose();\n\n\t} else {\n\n\t\t_m1.identity();\n\n\t}\n\n\treturn _m1;\n\n} ")});
    own(bindings, "backgroundRotation", rotation);

    auto module = JSValue::object(nullptr);
    auto names = bindings.ownPropertyNames();
    std::sort(names.begin(), names.end());
    for (const auto& name : names) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeScenePropertiesBinding(const std::string& name) {
    const auto keeper = nativeScenePropertiesModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

