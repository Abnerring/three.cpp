#include "nodes/accessors/Lights.h"

#include "core/JSWeakMap.h"
#include "math/Vector3.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/Position.h"
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

JSValue lightData(const JSValue& table, const JSValue& light) {
    JSWeakMap cache(table.get("uniformsLib"));
    auto data = cache.get(light);
    if (data.isUndefined()) {
        data = JSValue::object(nullptr);
        cache.set(light, data);
    }
    return data;
}

JSValue update(const JSValue&, const Args& args, const Args& captures) {
    const auto light = captures[0];
    const auto kind = captures[1].string();
    auto self = args.size() > 1 ? args[1] : JSValue{};
    if (kind == "shadowMatrix") {
        const auto frame = args[0];
        const auto castShadow = light.get("castShadow").strictEquals(true);
        const auto renderer = frame.get("renderer");
        const auto shadowMap = renderer.get("shadowMap");
        if (!castShadow || shadowMap.get("enabled").strictEquals(false)) {
            const auto shadow = light.get("shadow");
            auto camera = shadow.get("camera");
            if (!camera.get("coordinateSystem").strictEquals(frame.get("camera").get("coordinateSystem"))) {
                camera.set("coordinateSystem", frame.get("camera").get("coordinateSystem"));
                invoke(camera, "updateProjectionMatrix")();
            }
            invoke(shadow, "updateMatrices")({light});
            return shadow.get("matrix");
        }
        return light.get("shadow").get("matrix");
    }
    if (kind == "position") {
        return invoke(self.get("value"), "setFromMatrixPosition")({light.get("matrixWorld")});
    }
    if (kind == "targetPosition") {
        return invoke(self.get("value"), "setFromMatrixPosition")({light.get("target").get("matrixWorld")});
    }
    if (kind == "viewPosition") {
        auto value = self.get("value");
        if (value.isUndefined() || value.isNull()) {
            value = nativeVector3Constructor().construct();
            self.set("value", value);
        }
        invoke(value, "setFromMatrixPosition")({light.get("matrixWorld")});
        return invoke(value, "applyMatrix4")({args[0].get("camera").get("matrixWorldInverse")});
    }
    return {};
}

JSValue callback(const char* kind, const JSValue& light, const char* source, unsigned arity = 1) {
    auto fn = JSValue::function(update, {light, kind}, "", arity);
    fn.setFunctionSourceText(source);
    return fn;
}

JSValue makeUniform(const JSValue& table, const JSValue& value, const JSValue& light,
                    const char* kind, const char* source) {
    auto uniform = table.get("uniform").call({}, {value});
    uniform = invoke(uniform, "setGroup")({table.get("renderGroup")});
    return invoke(uniform, "onRenderUpdate")({callback(kind, light, source, 2)});
}

JSValue exportedFunction(const char* name, const JSValue& table);

JSValue lightExport(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    const auto name = captures[1].string();
    const auto light = arg(args, 0);
    auto data = lightData(table, light);

    if (name == "lightShadowMatrix") {
        auto value = data.get("shadowMatrix");
        if (value.isUndefined()) {
            value = makeUniform(table, "mat4", light, "shadowMatrix",
                "( frame ) => {\n\n\t\t// normally, shadow matrices are updated in ShadowNode. However, if the shadow matrix is used outside\n\t\t// of shadow rendering (like in ProjectorLightNode), the shadow matrix still requires an update\n\n\t\tif ( light.castShadow !== true || frame.renderer.shadowMap.enabled === false ) {\n\n\t\t\tif ( light.shadow.camera.coordinateSystem !== frame.camera.coordinateSystem ) {\n\n\t\t\t\tlight.shadow.camera.coordinateSystem = frame.camera.coordinateSystem;\n\t\t\t\tlight.shadow.camera.updateProjectionMatrix();\n\n\t\t\t}\n\n\t\t\tlight.shadow.updateMatrices( light );\n\n\t\t}\n\n\t\treturn light.shadow.matrix;\n\n\t} ");
            data.set("shadowMatrix", value);
        }
        return value;
    }
    if (name == "lightPosition") {
        auto value = data.get("position");
        if (value.isUndefined()) {
            value = makeUniform(table, nativeVector3Constructor().construct(), light, "position",
                "( _, self ) => self.value.setFromMatrixPosition( light.matrixWorld )");
            data.set("position", value);
        }
        return value;
    }
    if (name == "lightTargetPosition") {
        auto value = data.get("targetPosition");
        if (value.isUndefined()) {
            value = makeUniform(table, nativeVector3Constructor().construct(), light, "targetPosition",
                "( _, self ) => self.value.setFromMatrixPosition( light.target.matrixWorld )");
            data.set("targetPosition", value);
        }
        return value;
    }
    if (name == "lightProjectionUV") {
        auto position = args.size() > 1 ? args[1] : table.get("positionWorld");
        const auto spot = invoke(exportedFunction("lightShadowMatrix", table).call({}, {light}), "mul")({position});
        return invoke(spot.get("xyz"), "div")({spot.get("w")});
    }
    if (name == "lightTargetDirection") {
        const auto direction = invoke(exportedFunction("lightPosition", table).call({}, {light}), "sub")({
            exportedFunction("lightTargetPosition", table).call({}, {light})});
        return invoke(table.get("cameraViewMatrix"), "transformDirection")({direction});
    }
    if (name == "lightViewPosition") {
        auto value = data.get("viewPosition");
        if (value.isUndefined()) {
            auto body = JSValue::function(
                +[](const JSValue&, const Args& callArgs, const Args& captures) -> JSValue {
                    const auto input = callArgs.empty() ? JSValue::object(nullptr) : arg(callArgs, 0);
                    auto camera = input.get("camera");
                    if (camera.isUndefined() || camera.isNull()) camera = JSValue::object(nullptr);
                    const auto table = captures[0];
                    const auto light = captures[1];
                    auto data = captures[2];
                    if (camera.get("isArrayCamera").toBoolean() && camera.get("cameras").get("length").toNumber() > 0) {
                        const auto position = exportedFunction("lightPosition", table).call({}, {light});
                        return invoke(table.get("cameraViewMatrix"), "mul")({
                            table.get("vec4").call({}, {position, 1})}).get("xyz");
                    }
                    auto uniform = data.get("viewPositionUniform");
                    if (uniform.isUndefined()) {
                        uniform = makeUniform(table, nativeVector3Constructor().construct(), light, "viewPosition",
                            "( { camera }, self ) => {\n\n\t\t\tself.value = self.value || new Vector3();\n\t\t\tself.value.setFromMatrixPosition( light.matrixWorld );\n\n\t\t\tself.value.applyMatrix4( camera.matrixWorldInverse );\n\n\t\t}");
                        data.set("viewPositionUniform", uniform);
                    }
                    return uniform;
                }, {table, light, data}, "", 1);
            body.setFunctionSourceText("( { camera } ) => {\n\n\t// Each sub-camera of an array camera has its own view space.\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\treturn cameraViewMatrix.mul( vec4( lightPosition( light ), 1 ) ).xyz;\n\n\t}\n\n\treturn data.viewPositionUniform || ( data.viewPositionUniform = uniform( new Vector3() ).setGroup( renderGroup ).onRenderUpdate( ( { camera }, self ) => {\n\n\t\tself.value = self.value || new Vector3();\n\t\tself.value.setFromMatrixPosition( light.matrixWorld );\n\n\t\tself.value.applyMatrix4( camera.matrixWorldInverse );\n\n\t} ) );\n\n}");
            value = table.get("Fn").call({}, {body});
            value = invoke(value, "once")().call();
            data.set("viewPosition", value);
        }
        return value;
    }
    throw std::logic_error("Unknown Lights export: " + name);
}

JSValue exportedFunction(const char* name, const JSValue& table) {
    return table.get(name);
}

} // namespace

JSValue nativeLightsModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Lights module");

    nativeUniformGroupNodeModule();
    nativeTSLBase();
    nativeCameraModule();
    nativePositionModule();
    nativeVector3Constructor();

    auto table = JSValue::object(nullptr);
    own(table, "uniform", nativeUniformFactory());
    own(table, "renderGroup", nativeUniformGroupNodeBinding("renderGroup"));
    own(table, "Fn", nativeTSLBaseBinding("Fn"));
    own(table, "vec4", nativeTSLBaseBinding("vec4"));
    own(table, "cameraViewMatrix", nativeCameraBinding("cameraViewMatrix"));
    own(table, "positionWorld", nativePositionBinding("positionWorld"));
    own(table, "uniformsLib", createNativeWeakMap());
    own(table, "Vector3", nativeVector3Constructor());

    auto bindings = JSValue::object(nullptr);
    for (const auto* name : {"lightShadowMatrix", "lightProjectionUV", "lightPosition", "lightTargetPosition", "lightViewPosition", "lightTargetDirection"}) {
        const auto fn = JSValue::function(lightExport, {table, name}, name, 1);
        own(bindings, name, fn);
        own(table, name, fn);
    }
    bindings.get("lightShadowMatrix").setFunctionSourceText("function lightShadowMatrix( light ) {\n\n\tconst data = getLightData( light );\n\n\treturn data.shadowMatrix || ( data.shadowMatrix = uniform( 'mat4' ).setGroup( renderGroup ).onRenderUpdate( ( frame ) => { ... } ) );\n\n}");
    bindings.get("lightProjectionUV").setFunctionSourceText("function lightProjectionUV( light, position = positionWorld ) {\n\n\tconst spotLightCoord = lightShadowMatrix( light ).mul( position );\n\tconst projectionUV = spotLightCoord.xyz.div( spotLightCoord.w );\n\n\treturn projectionUV;\n\n}");
    bindings.get("lightPosition").setFunctionSourceText("function lightPosition( light ) {\n\n\tconst data = getLightData( light );\n\n\treturn data.position || ( data.position = uniform( new Vector3() ).setGroup( renderGroup ).onRenderUpdate( ( _, self ) => self.value.setFromMatrixPosition( light.matrixWorld ) ) );\n\n}");
    bindings.get("lightTargetPosition").setFunctionSourceText("function lightTargetPosition( light ) {\n\n\tconst data = getLightData( light );\n\n\treturn data.targetPosition || ( data.targetPosition = uniform( new Vector3() ).setGroup( renderGroup ).onRenderUpdate( ( _, self ) => self.value.setFromMatrixPosition( light.target.matrixWorld ) ) );\n\n}");
    bindings.get("lightViewPosition").setFunctionSourceText("function lightViewPosition( light ) {\n\n\tconst data = getLightData( light );\n\n\treturn data.viewPosition || ( data.viewPosition = ( Fn( ( { camera } ) => { ... } ).once() )() );\n\n}");
    bindings.get("lightTargetDirection").setFunctionSourceText("( light ) => cameraViewMatrix.transformDirection( lightPosition( light ).sub( lightTargetPosition( light ) ) )");

    auto module = JSValue::object(nullptr);
    auto names = bindings.ownPropertyNames();
    std::sort(names.begin(), names.end());
    for (const auto& name : names) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    table.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeLightsBinding(const std::string& name) {
    const auto keeper = nativeLightsModule();
    (void)keeper;
    return lexical().lock()->get(name);
}
